"""
Base Driver Module - Abstract base class for all IL/RL test equipment drivers.

This module defines the common interface that all equipment drivers must implement,
ensuring consistent behavior across VIAVI, Santec, and future equipment types.

Author: Menghui
Date: 2026-02-13
Project: VIAVI & Santec Equipment Integration into MIMS/Mlight/MAS
"""

from __future__ import annotations

import logging
import socket
import time
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from enum import Enum
from typing import Optional, List, Tuple


# ---------------------------------------------------------------------------
# Data Models
# ---------------------------------------------------------------------------

class ConnectionState(Enum):
    """Equipment connection state."""
    DISCONNECTED = "disconnected"
    CONNECTING = "connecting"
    CONNECTED = "connected"
    ERROR = "error"


class MeasurementMode(Enum):
    """Measurement mode."""
    REFERENCE = "reference"
    MEASUREMENT = "measurement"


class MeasurementState(Enum):
    """Current measurement state."""
    IDLE = "idle"
    RUNNING = "running"
    COMPLETE = "complete"
    ERROR = "error"


@dataclass
class MeasurementResult:
    """Single channel, single wavelength measurement result."""
    channel: int
    wavelength: float          # nm
    insertion_loss: float      # dB (IL)
    return_loss: float         # dB (RL)
    raw_data: list = field(default_factory=list)   # Original raw values from device


@dataclass
class DeviceInfo:
    """Device identification information."""
    manufacturer: str          # "VIAVI" or "Santec"
    model: str                 # e.g. "MAP300", "TSL-570", etc.
    serial_number: str = ""
    firmware_version: str = ""
    slot: int = 0              # Slot number (if applicable)


@dataclass
class ConnectionConfig:
    """Connection configuration."""
    ip_address: str
    port: int
    timeout: float = 3.0       # seconds
    buffer_size: int = 1024
    reconnect_attempts: int = 3
    reconnect_delay: float = 2.0  # seconds


# ---------------------------------------------------------------------------
# Abstract Base Driver
# ---------------------------------------------------------------------------

class BaseEquipmentDriver(ABC):
    """
    Abstract base class for IL/RL test equipment drivers.

    All equipment drivers (VIAVI, Santec, etc.) must inherit from this class
    and implement the abstract methods.

    This class provides:
    - Common TCP socket connection management (connect/disconnect/reconnect)
    - Unified logging
    - Standard measurement workflow template

    Subclasses must implement:
    - _initialize_device()
    - _configure_measurement()
    - _execute_reference()
    - _execute_measurement()
    - _parse_results()
    - get_device_info()
    """

    def __init__(self, config: ConnectionConfig, device_type: str = "Unknown"):
        self.config = config
        self.device_type = device_type
        self._socket: Optional[socket.socket] = None
        self._state = ConnectionState.DISCONNECTED
        self._measurement_state = MeasurementState.IDLE

        # Setup logging
        self.logger = logging.getLogger(f"Driver.{device_type}")
        self.logger.setLevel(logging.DEBUG)
        if not self.logger.handlers:
            handler = logging.StreamHandler()
            formatter = logging.Formatter(
                "%(asctime)s [%(name)s] %(levelname)s: %(message)s"
            )
            handler.setFormatter(formatter)
            self.logger.addHandler(handler)

    # -----------------------------------------------------------------------
    # Connection Management
    # -----------------------------------------------------------------------

    @property
    def is_connected(self) -> bool:
        """Check if the device is currently connected."""
        return self._state == ConnectionState.CONNECTED

    def connect(self) -> bool:
        """
        Establish connection to the device.

        Returns:
            True if connection was successful, False otherwise.
        """
        if self.is_connected:
            self.logger.warning("Already connected. Disconnect first.")
            return True

        self._state = ConnectionState.CONNECTING
        for attempt in range(1, self.config.reconnect_attempts + 1):
            try:
                self.logger.info(
                    f"Connecting to {self.config.ip_address}:{self.config.port} "
                    f"(attempt {attempt}/{self.config.reconnect_attempts})"
                )
                self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self._socket.connect((self.config.ip_address, self.config.port))
                self._socket.settimeout(self.config.timeout)
                self._state = ConnectionState.CONNECTED
                self.logger.info("Connection established successfully.")
                return True
            except (socket.error, OSError) as e:
                self.logger.error(f"Connection attempt {attempt} failed: {e}")
                self._cleanup_socket()
                if attempt < self.config.reconnect_attempts:
                    time.sleep(self.config.reconnect_delay)

        self._state = ConnectionState.ERROR
        self.logger.error("All connection attempts exhausted.")
        return False

    def disconnect(self):
        """Disconnect from the device."""
        if self._socket:
            try:
                self._socket.close()
                self.logger.info("Disconnected from device.")
            except OSError as e:
                self.logger.error(f"Error during disconnect: {e}")
            finally:
                self._cleanup_socket()
        self._state = ConnectionState.DISCONNECTED

    def reconnect(self) -> bool:
        """Disconnect and reconnect to the device."""
        self.logger.info("Reconnecting...")
        self.disconnect()
        time.sleep(self.config.reconnect_delay)
        return self.connect()

    def _cleanup_socket(self):
        """Clean up socket resources."""
        if self._socket:
            try:
                self._socket.close()
            except OSError:
                pass
            self._socket = None

    # -----------------------------------------------------------------------
    # Low-level Communication
    # -----------------------------------------------------------------------

    def send_command(
        self,
        command: str,
        end_char: str = "\n",
        expect_response: bool = False,
        buffer_size: Optional[int] = None,
    ) -> Optional[str]:
        """
        Send a command to the device and optionally receive a response.

        Args:
            command: The command string to send.
            end_char: Character appended to the command (default: newline).
            expect_response: Whether to wait for a response.
            buffer_size: Override default buffer size for this command.

        Returns:
            Response string if expect_response=True, else None.

        Raises:
            ConnectionError: If not connected to the device.
            TimeoutError: If response times out.
        """
        if not self.is_connected or not self._socket:
            raise ConnectionError("Not connected to device.")

        buf_size = buffer_size or self.config.buffer_size
        full_command = f"{command}{end_char}"

        self.logger.debug(f"TX: {command}")
        try:
            self._socket.send(full_command.encode("utf-8"))
        except (socket.error, OSError) as e:
            self._state = ConnectionState.ERROR
            raise ConnectionError(f"Failed to send command: {e}") from e

        if expect_response or "?" in command:
            return self._receive_response(buf_size)
        return None

    def _receive_response(self, buffer_size: int) -> str:
        """
        Receive a response from the device until a newline is found.

        Args:
            buffer_size: Size of the receive buffer.

        Returns:
            Decoded response string (without trailing newline).
        """
        data = ""
        try:
            while True:
                chunk = self._socket.recv(buffer_size).decode("ASCII")
                data += chunk
                if "\n" in data:
                    break
        except socket.timeout:
            self.logger.warning(f"Response timeout. Partial data: {data!r}")
            raise TimeoutError(f"Response timeout. Partial: {data!r}")
        except (socket.error, OSError) as e:
            self._state = ConnectionState.ERROR
            raise ConnectionError(f"Receive error: {e}") from e

        response = data.strip()
        self.logger.debug(f"RX: {response}")
        return response

    # -----------------------------------------------------------------------
    # Error Handling
    # -----------------------------------------------------------------------

    @abstractmethod
    def check_error(self) -> tuple[int, str]:
        """
        Query the device for the last error.

        Returns:
            Tuple of (error_code, error_message). code=0 means no error.
        """
        ...

    def assert_no_error(self):
        """Check for errors and raise if any found."""
        code, message = self.check_error()
        if code != 0:
            self.logger.error(f"Device error [{code}]: {message}")
            raise RuntimeError(f"Device error [{code}]: {message}")

    # -----------------------------------------------------------------------
    # Abstract Methods - Must be implemented by subclasses
    # -----------------------------------------------------------------------

    @abstractmethod
    def get_device_info(self) -> DeviceInfo:
        """Get device identification information."""
        ...

    @abstractmethod
    def initialize(self) -> bool:
        """
        Initialize the device after connection.
        e.g., launch applications, set default parameters, etc.

        Returns:
            True if initialization was successful.
        """
        ...

    @abstractmethod
    def configure_wavelengths(self, wavelengths: list[float]):
        """
        Configure measurement wavelengths.

        Args:
            wavelengths: List of wavelengths in nm (e.g., [1310.0, 1550.0]).
        """
        ...

    @abstractmethod
    def configure_channels(self, channels: list[int]):
        """
        Configure measurement channels.

        Args:
            channels: List of channel numbers (e.g., [1, 2, 3, ...12]).
        """
        ...

    @abstractmethod
    def take_reference(self, override: bool = False, **kwargs) -> bool:
        """
        Perform a reference (zero-loss) measurement.

        Args:
            override: If True, use override values instead of actual reference.
            **kwargs: Additional parameters (e.g., il_value, length_value).

        Returns:
            True if reference was successful.
        """
        ...

    @abstractmethod
    def take_measurement(self) -> bool:
        """
        Perform an IL/RL measurement.

        Returns:
            True if measurement was successful.
        """
        ...

    @abstractmethod
    def get_results(self) -> list[MeasurementResult]:
        """
        Retrieve measurement results for all channels and wavelengths.

        Returns:
            List of MeasurementResult objects.
        """
        ...

    # -----------------------------------------------------------------------
    # High-level Workflow (Template Method Pattern)
    # -----------------------------------------------------------------------

    def run_full_test(
        self,
        wavelengths: list[float],
        channels: list[int],
        do_reference: bool = True,
        override_reference: bool = False,
        **ref_kwargs,
    ) -> list[MeasurementResult]:
        """
        Run a complete test workflow: configure -> reference -> measure -> results.

        This is a template method that orchestrates the full test sequence
        using the abstract methods implemented by each driver.

        Args:
            wavelengths: List of wavelengths to test (nm).
            channels: List of channel numbers to test.
            do_reference: Whether to take a reference measurement first.
            override_reference: Whether to override reference with manual values.
            **ref_kwargs: Additional reference parameters.

        Returns:
            List of MeasurementResult objects.
        """
        self.logger.info("=== Starting Full Test Workflow ===")

        # Step 1: Configure
        self.logger.info(f"Configuring wavelengths: {wavelengths}")
        self.configure_wavelengths(wavelengths)
        self.assert_no_error()

        self.logger.info(f"Configuring channels: {channels}")
        self.configure_channels(channels)
        self.assert_no_error()

        # Step 2: Reference (optional)
        if do_reference:
            self.logger.info(
                f"Taking reference (override={override_reference})..."
            )
            success = self.take_reference(
                override=override_reference, **ref_kwargs
            )
            if not success:
                raise RuntimeError("Reference measurement failed.")
            self.assert_no_error()

        # Step 3: Measurement
        self.logger.info("Taking measurement...")
        success = self.take_measurement()
        if not success:
            raise RuntimeError("Measurement failed.")
        self.assert_no_error()

        # Step 4: Retrieve results
        self.logger.info("Retrieving results...")
        results = self.get_results()
        self.logger.info(f"=== Test Complete: {len(results)} results ===")

        return results

    # -----------------------------------------------------------------------
    # Data Formatting (for MIMS/Mlight/MAS integration)
    # -----------------------------------------------------------------------

    @staticmethod
    def format_results_for_mims(results: list[MeasurementResult]) -> list[dict]:
        """
        Format measurement results into the structure expected by MIMS.

        Args:
            results: List of MeasurementResult objects.

        Returns:
            List of dicts in MIMS-compatible format.
        """
        # TODO: Adjust format based on actual MIMS data schema
        formatted = []
        for r in results:
            formatted.append({
                "channel": r.channel,
                "wavelength_nm": r.wavelength,
                "IL_dB": round(r.insertion_loss, 4),
                "RL_dB": round(r.return_loss, 4),
                "raw": r.raw_data,
            })
        return formatted

    @staticmethod
    def format_results_for_mas(results: list[MeasurementResult]) -> list[dict]:
        """
        Format measurement results for MAS data upload.

        Args:
            results: List of MeasurementResult objects.

        Returns:
            List of dicts in MAS-compatible format.
        """
        # TODO: Adjust format based on actual MAS data upload schema
        formatted = []
        for r in results:
            formatted.append({
                "ch": r.channel,
                "wl": r.wavelength,
                "il": round(r.insertion_loss, 4),
                "rl": round(r.return_loss, 4),
            })
        return formatted

    # -----------------------------------------------------------------------
    # Context Manager Support
    # -----------------------------------------------------------------------

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()
        return False

    def __repr__(self):
        return (
            f"<{self.__class__.__name__} "
            f"type={self.device_type} "
            f"addr={self.config.ip_address}:{self.config.port} "
            f"state={self._state.value}>"
        )

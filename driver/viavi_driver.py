"""
VIAVI MAP300 PCT Driver - Concrete driver implementation for VIAVI MAP300 PCT.

Implements the BaseEquipmentDriver interface for controlling VIAVI MAP300 PCT
(Passive Component Tester) equipment via TCP socket with SCPI-like commands.

Based on the communication protocol from PCT_12_Channel_Test example and
VIAVI MAP300 documentation.

Author: Menghui
Date: 2026-02-13
Project: VIAVI & Santec Equipment Integration into MIMS/Mlight/MAS
"""

from __future__ import annotations

import time
from typing import Optional, List

from base_driver import (
    BaseEquipmentDriver,
    ConnectionConfig,
    DeviceInfo,
    MeasurementMode,
    MeasurementResult,
    MeasurementState,
)


class ViaviPCTDriver(BaseEquipmentDriver):
    """
    Driver for VIAVI MAP300 PCT (Passive Component Tester).

    The MAP300 uses a two-level connection architecture:
    1. Chassis connection (port 8100) - for system management
    2. PCT Module connection (port 8300 + slot) - for measurement control

    Communication uses SCPI-like commands over TCP sockets.
    """

    # Chassis port is always 8100
    CHASSIS_PORT = 8100
    # PCT module base port
    PCT_BASE_PORT = 8300
    # Super Application startup wait time (seconds)
    SUPER_APP_STARTUP_WAIT = 10
    # Measurement polling interval (seconds)
    POLL_INTERVAL = 0.1
    # Maximum polling time before timeout (seconds)
    MAX_POLL_TIME = 300

    def __init__(
        self,
        ip_address: str,
        slot: int = 1,
        timeout: float = 3.0,
        launch_port: int = 1,
    ):
        """
        Initialize VIAVI MAP300 PCT driver.

        Args:
            ip_address: IP address of the MAP300 chassis.
            slot: Slot number of the PCT module (determines port).
            timeout: Socket timeout in seconds.
            launch_port: Optical launch port (e.g., 1 for J1).
        """
        pct_port = self.PCT_BASE_PORT + slot
        config = ConnectionConfig(
            ip_address=ip_address,
            port=pct_port,
            timeout=timeout,
        )
        super().__init__(config, device_type="VIAVI_MAP300_PCT")

        self._ip_address = ip_address
        self._slot = slot
        self._launch_port = launch_port
        self._chassis_socket = None
        self._wavelengths: list[float] = []
        self._channels: list[int] = []

    # -----------------------------------------------------------------------
    # Connection Management (Override for two-level connection)
    # -----------------------------------------------------------------------

    def connect(self) -> bool:
        """
        Establish two-level connection:
        1. Connect to Chassis
        2. Launch Super Application
        3. Connect to PCT Module
        """
        # Step 1: Connect to Chassis
        self.logger.info(
            f"Connecting to Chassis at {self._ip_address}:{self.CHASSIS_PORT}"
        )
        try:
            import socket as sock_module

            self._chassis_socket = sock_module.socket(
                sock_module.AF_INET, sock_module.SOCK_STREAM
            )
            self._chassis_socket.connect(
                (self._ip_address, self.CHASSIS_PORT)
            )
            self._chassis_socket.settimeout(self.config.timeout)
            self.logger.info("Chassis connection established.")
        except (OSError, Exception) as e:
            self.logger.error(f"Chassis connection failed: {e}")
            return False

        # Step 2: Launch Super Application
        self.logger.info("Launching Super Application (PCT)...")
        self._send_chassis_command("SUPER:LAUNCH PCT")
        self.logger.info(
            f"Waiting {self.SUPER_APP_STARTUP_WAIT}s for Super App to start..."
        )
        time.sleep(self.SUPER_APP_STARTUP_WAIT)

        # Step 3: Connect to PCT Module
        self.logger.info(
            f"Connecting to PCT Module at slot {self._slot} "
            f"(port {self.config.port})"
        )
        return super().connect()

    def disconnect(self):
        """Disconnect from both PCT Module and Chassis."""
        super().disconnect()
        if self._chassis_socket:
            try:
                self._chassis_socket.close()
                self.logger.info("Chassis connection closed.")
            except OSError:
                pass
            self._chassis_socket = None

    def _send_chassis_command(self, command: str) -> Optional[str]:
        """
        Send a command to the Chassis (not the PCT module).

        Args:
            command: Command string.

        Returns:
            Response if query command, else None.
        """
        if not self._chassis_socket:
            raise ConnectionError("Not connected to chassis.")

        full_cmd = f"{command}\n"
        self.logger.debug(f"TX (Chassis): {command}")
        self._chassis_socket.send(full_cmd.encode("utf-8"))

        if "?" in command:
            data = ""
            while "\n" not in data:
                data += self._chassis_socket.recv(
                    self.config.buffer_size
                ).decode("ASCII")
            response = data.strip()
            self.logger.debug(f"RX (Chassis): {response}")
            return response
        return None

    # -----------------------------------------------------------------------
    # Error Handling
    # -----------------------------------------------------------------------

    def check_error(self) -> tuple[int, str]:
        """Query the device for the last error via SYST:ERR? command."""
        response = self.send_command("SYST:ERR?")
        if response:
            parts = response.split(",", 1)
            code = int(parts[0].strip())
            message = parts[1].strip() if len(parts) > 1 else ""
            return code, message
        return 0, "No response"

    # -----------------------------------------------------------------------
    # Device Information
    # -----------------------------------------------------------------------

    def get_device_info(self) -> DeviceInfo:
        """Get VIAVI MAP300 PCT device information."""
        return DeviceInfo(
            manufacturer="VIAVI",
            model="MAP300 PCT",
            slot=self._slot,
            # TODO: Query actual serial/firmware if supported via *IDN? or similar
        )

    # -----------------------------------------------------------------------
    # Initialization
    # -----------------------------------------------------------------------

    def initialize(self) -> bool:
        """
        Initialize PCT after connection.
        The Super Application is already launched during connect().
        Additional initialization can be added here.
        """
        self.logger.info("Initializing VIAVI MAP300 PCT...")
        try:
            # Set launch port
            self.send_command(f"PATH:LAUNCH {self._launch_port}")
            self.assert_no_error()
            self.logger.info(
                f"Launch port set to J{self._launch_port}."
            )
            return True
        except Exception as e:
            self.logger.error(f"Initialization failed: {e}")
            return False

    # -----------------------------------------------------------------------
    # Configuration
    # -----------------------------------------------------------------------

    def configure_wavelengths(self, wavelengths: list[float]):
        """Configure measurement wavelengths (e.g., [1310, 1550])."""
        self._wavelengths = wavelengths
        wl_str = ", ".join(str(int(w)) for w in wavelengths)
        self.send_command(f"SOURCE:LIST {wl_str}")
        self.assert_no_error()

        # Verify
        response = self.send_command("SOURCE:LIST?")
        actual = [float(w.strip()) for w in response.split(",")]
        self.logger.info(f"Wavelengths configured: {actual} nm")

    def configure_channels(self, channels: list[int]):
        """Configure measurement channels (e.g., [1,2,...,12])."""
        self._channels = channels
        if not channels:
            return

        # Build channel range string (e.g., "1-12" or "1,3,5")
        ch_str = self._build_channel_string(channels)
        self.send_command(
            f"PATH:LIST {self._launch_port},{ch_str}"
        )
        self.assert_no_error()
        self.logger.info(f"Channels configured: {channels}")

    def configure_orl(
        self,
        method: int = 2,
        origin: int = 1,
        a_offset: float = -0.5,
        b_offset: float = 0.5,
    ):
        """
        Configure ORL (Optical Return Loss) measurement window.

        Args:
            method: 1=Integration Mode, 2=Discrete Mode (recommended for APC).
            origin: 1=A+B from DUT start, 2=A+B from DUT end, 3=A start/B end.
            a_offset: A marker offset in meters.
            b_offset: B marker offset in meters.
        """
        self.send_command(
            f"MEAS:ORL:SETUP 1,{method},{origin},{a_offset},{b_offset}"
        )
        self.assert_no_error()
        self.logger.info(
            f"ORL configured: method={method}, origin={origin}, "
            f"A={a_offset}m, B={b_offset}m"
        )

    # -----------------------------------------------------------------------
    # Reference Measurement
    # -----------------------------------------------------------------------

    def take_reference(
        self,
        override: bool = False,
        il_value: float = 0.1,
        length_value: float = 3.0,
        **kwargs,
    ) -> bool:
        """
        Take a reference (zero-loss) measurement.

        Args:
            override: If True, use manual override values.
            il_value: Override IL value in dB (used if override=True).
            length_value: Override fiber length in meters (used if override=True).
        """
        if override:
            return self._override_reference(il_value, length_value)
        else:
            return self._auto_reference()

    def _override_reference(
        self, il_value: float, length_value: float
    ) -> bool:
        """Apply manual reference override for all channels."""
        self.logger.info(
            f"Overriding reference: IL={il_value}dB, Length={length_value}m"
        )
        for ch in self._channels:
            # Set IL override
            self.send_command(
                f"PATH:JUMPER:IL {self._launch_port},{ch},{il_value}"
            )
            self.assert_no_error()
            # Activate IL override (0 = use override)
            self.send_command(
                f"PATH:JUMPER:IL:AUTO {self._launch_port},{ch},0"
            )
            self.assert_no_error()

            # Set length override
            self.send_command(
                f"PATH:JUMPER:LENGTH {self._launch_port},{ch},{length_value}"
            )
            self.assert_no_error()
            # Activate length override (0 = use override)
            self.send_command(
                f"PATH:JUMPER:LENGTH:AUTO {self._launch_port},{ch},0"
            )
            self.assert_no_error()

        self.logger.info("Reference override applied for all channels.")
        return True

    def _auto_reference(self) -> bool:
        """Perform automatic reference measurement."""
        self.logger.info("Starting automatic reference measurement...")

        # Deactivate overrides
        for ch in self._channels:
            self.send_command(
                f"PATH:JUMPER:IL:AUTO {self._launch_port},{ch},1"
            )
            self.assert_no_error()
            self.send_command(
                f"PATH:JUMPER:LENGTH:AUTO {self._launch_port},{ch},1"
            )
            self.assert_no_error()

        # Set to reference mode
        self.send_command("SENS:FUNC 0")
        self.assert_no_error()

        # Start reference
        self.send_command("MEAS:START")
        return self._wait_for_completion("Reference")

    # -----------------------------------------------------------------------
    # Measurement
    # -----------------------------------------------------------------------

    def take_measurement(self) -> bool:
        """Perform IL/RL measurement."""
        self.logger.info("Starting IL/RL measurement...")

        # Set to measurement mode
        self.send_command("SENS:FUNC 1")
        self.assert_no_error()

        # Start measurement
        self.send_command("MEAS:START")
        return self._wait_for_completion("Measurement")

    def _wait_for_completion(self, operation_name: str) -> bool:
        """
        Poll MEAS:STATE? until measurement completes or errors.

        Args:
            operation_name: Name for logging (e.g., "Reference", "Measurement").

        Returns:
            True if completed successfully.
        """
        start_time = time.time()
        while True:
            state = self.send_command("MEAS:STATE?")

            if state == "1":  # Complete
                self.logger.info(f"{operation_name} completed successfully.")
                self.assert_no_error()
                return True
            elif state == "3":  # Error
                self.logger.error(f"{operation_name} encountered an error.")
                code, msg = self.check_error()
                self.logger.error(f"Error [{code}]: {msg}")
                return False
            elif state == "2":  # Still running
                elapsed = time.time() - start_time
                if elapsed > self.MAX_POLL_TIME:
                    self.logger.error(
                        f"{operation_name} timed out after {elapsed:.1f}s."
                    )
                    return False
                time.sleep(self.POLL_INTERVAL)
            else:
                self.logger.warning(
                    f"Unexpected state: {state!r}"
                )
                time.sleep(self.POLL_INTERVAL)

    # -----------------------------------------------------------------------
    # Results Retrieval
    # -----------------------------------------------------------------------

    def get_results(self) -> list[MeasurementResult]:
        """
        Retrieve IL/RL results for all configured channels and wavelengths.

        The MEAS:ALL? command returns 5 values per wavelength per channel:
        [val1, val2, val3, RL, val5] repeated for each wavelength.
        """
        results = []

        # Get current wavelength list for result mapping
        wl_response = self.send_command("SOURCE:LIST?")
        wavelengths = [float(w.strip()) for w in wl_response.split(",")]

        for ch in self._channels:
            response = self.send_command(
                f"MEAS:ALL? {ch},{self._launch_port}"
            )
            raw_values = response.split(",")

            for wl_idx, wavelength in enumerate(wavelengths):
                # Each wavelength has 5 values
                start = wl_idx * 5
                end = start + 5
                segment = raw_values[start:end]

                if len(segment) >= 5:
                    # Index 3 = RL, index mapping for IL may vary
                    # Based on example: RL is at index 3
                    rl = float(segment[3])
                    # TODO: Confirm IL index from full documentation
                    # Tentatively using index 0 or 1 for IL
                    il = float(segment[0]) if segment[0] else 0.0

                    results.append(
                        MeasurementResult(
                            channel=ch,
                            wavelength=wavelength,
                            insertion_loss=il,
                            return_loss=rl,
                            raw_data=[float(v) for v in segment],
                        )
                    )
                else:
                    self.logger.warning(
                        f"Incomplete data for ch={ch}, wl={wavelength}: "
                        f"{segment}"
                    )

            self.assert_no_error()

        self.logger.info(f"Retrieved {len(results)} measurement results.")
        return results

    # -----------------------------------------------------------------------
    # Helper Methods
    # -----------------------------------------------------------------------

    @staticmethod
    def _build_channel_string(channels: list[int]) -> str:
        """
        Build channel string for PATH:LIST command.
        Tries to build range notation (e.g., "1-12") when possible.
        """
        if not channels:
            return ""

        channels_sorted = sorted(channels)

        # Check if it's a contiguous range
        if channels_sorted == list(
            range(channels_sorted[0], channels_sorted[-1] + 1)
        ):
            return f"{channels_sorted[0]}-{channels_sorted[-1]}"

        # Otherwise, comma-separated
        return ",".join(str(ch) for ch in channels_sorted)


# ---------------------------------------------------------------------------
# Usage Example
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    """
    Example usage of the VIAVI PCT Driver.

    This mirrors the workflow from PCT_12_Channel_Test.py
    but uses the structured driver framework.
    """
    logging.basicConfig(level=logging.INFO)

    # Configuration
    IP = "10.14.132.194"
    SLOT = 1

    # Create and use driver with context manager
    driver = ViaviPCTDriver(ip_address=IP, slot=SLOT)

    try:
        # Connect (handles chassis + super app + PCT module)
        driver.connect()

        # Initialize (set launch port)
        driver.initialize()

        # Configure ORL window
        driver.configure_orl(method=2, origin=1, a_offset=-0.5, b_offset=0.5)

        # Run full test workflow
        results = driver.run_full_test(
            wavelengths=[1310, 1550],
            channels=list(range(1, 13)),  # Channels 1-12
            do_reference=True,
            override_reference=True,
            il_value=0.1,
            length_value=3.0,
        )

        # Display results
        print("\n" + "=" * 60)
        print("MEASUREMENT RESULTS")
        print("=" * 60)
        for r in results:
            print(
                f"  Channel {r.channel:2d} @ {r.wavelength:.0f}nm: "
                f"IL={r.insertion_loss:.4f}dB  RL={r.return_loss:.4f}dB"
            )

        # Format for MIMS
        mims_data = driver.format_results_for_mims(results)
        print(f"\nMIMS formatted: {len(mims_data)} records ready for upload.")

    finally:
        driver.disconnect()

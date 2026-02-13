"""
=============================================================================
  VIAVI & Santec IL/RL Test Equipment - Integration Test Demo
=============================================================================

This demo demonstrates the full driver workflow:
  1. Connect to equipment (simulated via built-in Mock Server)
  2. Initialize device
  3. Configure wavelengths, channels, ORL
  4. Take reference measurement
  5. Take IL/RL measurement
  6. Retrieve & display results
  7. Format data for MIMS / MAS upload
  8. Export results to CSV

Can run in two modes:
  - SIMULATION mode (default): Uses built-in mock TCP server, no hardware needed
  - LIVE mode: Connects to real VIAVI MAP300 equipment

Usage:
  python test_demo.py                    # Simulation mode
  python test_demo.py --live 10.14.132.194 --slot 1   # Live mode

Author: Menghui
Date: 2026-02-13
"""

from __future__ import annotations

import sys
import os
import csv
import json
import time
import socket
import logging
import threading
import argparse
import random
from datetime import datetime
from pathlib import Path
from typing import Optional, List

# Add driver package to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "driver"))

from base_driver import MeasurementResult, ConnectionState
from viavi_driver import ViaviPCTDriver
from santec_driver import SantecDriver
from driver_factory import DriverFactory, create_driver


# =============================================================================
# Mock VIAVI MAP300 TCP Server (Simulation)
# =============================================================================

class MockViaviServer:
    """
    Simulates a VIAVI MAP300 PCT chassis + module via TCP sockets.

    Accepts SCPI-like commands and returns realistic mock responses,
    allowing the full driver workflow to be tested without hardware.
    """

    def __init__(self, host: str = "127.0.0.1", chassis_port: int = 8100,
                 pct_port: int = 8301):
        self.host = host
        self.chassis_port = chassis_port
        self.pct_port = pct_port
        self._running = False
        self._threads: list[threading.Thread] = []

        # Simulated device state
        self._wavelengths = [1310.0, 1550.0]
        self._channels = list(range(1, 13))
        self._mode = 1           # 0=reference, 1=measurement
        self._meas_state = 0     # 0=idle, 1=complete, 2=running
        self._super_launched = False

    def start(self):
        """Start the mock server (chassis + PCT module listeners)."""
        self._running = True

        chassis_thread = threading.Thread(
            target=self._run_listener,
            args=(self.chassis_port, self._handle_chassis),
            daemon=True,
        )
        pct_thread = threading.Thread(
            target=self._run_listener,
            args=(self.pct_port, self._handle_pct),
            daemon=True,
        )

        chassis_thread.start()
        pct_thread.start()
        self._threads = [chassis_thread, pct_thread]

        # Give servers time to bind
        time.sleep(0.3)
        return self

    def stop(self):
        """Stop the mock server."""
        self._running = False

    def _run_listener(self, port: int, handler):
        """Run a TCP listener on the given port."""
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.settimeout(1.0)
        server.bind((self.host, port))
        server.listen(2)

        while self._running:
            try:
                conn, addr = server.accept()
                client_thread = threading.Thread(
                    target=self._handle_client,
                    args=(conn, handler),
                    daemon=True,
                )
                client_thread.start()
            except socket.timeout:
                continue
            except OSError:
                break
        server.close()

    def _handle_client(self, conn: socket.socket, handler):
        """Handle a connected client."""
        conn.settimeout(1.0)
        buffer = ""
        while self._running:
            try:
                data = conn.recv(1024).decode("utf-8")
                if not data:
                    break
                buffer += data
                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    line = line.strip()
                    if line:
                        response = handler(line)
                        if response is not None:
                            conn.send(f"{response}\n".encode("utf-8"))
            except socket.timeout:
                continue
            except (OSError, ConnectionResetError):
                break
        conn.close()

    def _handle_chassis(self, command: str) -> Optional[str]:
        """Handle chassis-level commands."""
        if command == "SUPER:LAUNCH PCT":
            self._super_launched = True
            return None  # No response for set commands
        return None

    def _handle_pct(self, command: str) -> Optional[str]:
        """Handle PCT module commands with realistic simulated responses."""
        cmd = command.strip().upper()

        # --- Error query ---
        if cmd == "SYST:ERR?":
            return "0,No error"

        # --- Source/Wavelength ---
        if cmd.startswith("SOURCE:LIST ") and "?" not in cmd:
            wl_str = cmd.replace("SOURCE:LIST ", "")
            self._wavelengths = [float(w.strip()) for w in wl_str.split(",")]
            return None
        if cmd == "SOURCE:LIST?":
            return ",".join(str(int(w)) for w in self._wavelengths)

        # --- Path/Channel ---
        if cmd.startswith("PATH:LAUNCH"):
            return None
        if cmd.startswith("PATH:LIST"):
            return None
        if cmd.startswith("PATH:JUMPER"):
            return None

        # --- ORL Setup ---
        if cmd.startswith("MEAS:ORL:SETUP"):
            return None

        # --- Measurement mode ---
        if cmd.startswith("SENS:FUNC"):
            parts = cmd.split()
            if len(parts) > 1:
                self._mode = int(parts[1])
            return None

        # --- Measurement control ---
        if cmd == "MEAS:START":
            self._meas_state = 1  # Immediately complete for simulation
            return None
        if cmd == "MEAS:STATE?":
            return str(self._meas_state)  # 1 = complete

        # --- Results ---
        if cmd.startswith("MEAS:ALL?"):
            return self._generate_mock_results()

        # Default: no response
        return None

    def _generate_mock_results(self) -> str:
        """
        Generate realistic mock IL/RL measurement data.

        Format: 5 values per wavelength (IL, val2, val3, RL, val5)
        Typical IL range: 0.1 - 0.5 dB
        Typical RL range: 50 - 65 dB (good connector)
        """
        values = []
        for wl in self._wavelengths:
            # Simulate realistic optical measurement values
            il = round(random.uniform(0.05, 0.45), 4)       # IL in dB
            power_in = round(random.uniform(-3.0, -1.0), 4)  # Input power
            power_out = round(power_in - il, 4)               # Output power
            rl = round(random.uniform(50.0, 65.0), 2)        # RL in dB
            orl = round(random.uniform(55.0, 70.0), 2)       # ORL in dB
            values.extend([il, power_in, power_out, rl, orl])
        return ",".join(str(v) for v in values)


# =============================================================================
# Result Formatting & Export Utilities
# =============================================================================

def print_banner(title: str, width: int = 70):
    """Print a formatted banner."""
    print("\n" + "=" * width)
    print(f"  {title}")
    print("=" * width)


def print_section(title: str, width: int = 70):
    """Print a section header."""
    print(f"\n--- {title} {'-' * (width - len(title) - 5)}")


def format_results_table(results: list[MeasurementResult]) -> str:
    """Format results as a readable ASCII table."""
    lines = []
    lines.append(f"{'Ch':>4} | {'Wavelength':>12} | {'IL (dB)':>10} | {'RL (dB)':>10} | {'Status':>8}")
    lines.append("-" * 60)

    for r in results:
        # Simple pass/fail based on typical specs
        il_pass = r.insertion_loss < 0.5
        rl_pass = r.return_loss > 50.0
        status = "PASS" if (il_pass and rl_pass) else "FAIL"

        lines.append(
            f"{r.channel:>4} | {r.wavelength:>10.0f}nm | "
            f"{r.insertion_loss:>10.4f} | {r.return_loss:>10.2f} | "
            f"{status:>8}"
        )

    return "\n".join(lines)


def export_results_csv(results: list[MeasurementResult], filepath: str):
    """Export measurement results to a CSV file."""
    with open(filepath, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow([
            "Channel", "Wavelength_nm", "IL_dB", "RL_dB",
            "IL_Pass", "RL_Pass", "Overall", "Timestamp",
        ])
        timestamp = datetime.now().isoformat()
        for r in results:
            il_pass = "PASS" if r.insertion_loss < 0.5 else "FAIL"
            rl_pass = "PASS" if r.return_loss > 50.0 else "FAIL"
            overall = "PASS" if (il_pass == "PASS" and rl_pass == "PASS") else "FAIL"
            writer.writerow([
                r.channel, r.wavelength, round(r.insertion_loss, 4),
                round(r.return_loss, 2), il_pass, rl_pass, overall, timestamp,
            ])
    return filepath


def export_results_json(results: list[MeasurementResult], filepath: str):
    """Export measurement results to a JSON file (MIMS-compatible)."""
    data = {
        "test_info": {
            "timestamp": datetime.now().isoformat(),
            "equipment": "VIAVI MAP300 PCT",
            "total_channels": len(set(r.channel for r in results)),
            "wavelengths": sorted(set(r.wavelength for r in results)),
            "total_results": len(results),
        },
        "results": [
            {
                "channel": r.channel,
                "wavelength_nm": r.wavelength,
                "IL_dB": round(r.insertion_loss, 4),
                "RL_dB": round(r.return_loss, 2),
                "raw_data": r.raw_data,
                "pass": r.insertion_loss < 0.5 and r.return_loss > 50.0,
            }
            for r in results
        ],
        "summary": {
            "total_pass": sum(
                1 for r in results
                if r.insertion_loss < 0.5 and r.return_loss > 50.0
            ),
            "total_fail": sum(
                1 for r in results
                if not (r.insertion_loss < 0.5 and r.return_loss > 50.0)
            ),
            "avg_IL_dB": round(
                sum(r.insertion_loss for r in results) / len(results), 4
            ) if results else 0,
            "avg_RL_dB": round(
                sum(r.return_loss for r in results) / len(results), 2
            ) if results else 0,
        },
    }
    with open(filepath, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    return filepath


# =============================================================================
# Demo Scenarios
# =============================================================================

def demo_viavi_full_test(ip: str, slot: int, simulation: bool = True):
    """
    Demo 1: VIAVI MAP300 PCT - Full 12-Channel IL/RL Test

    Demonstrates the complete workflow:
    connect -> init -> configure -> reference -> measure -> results -> export
    """
    print_banner("Demo 1: VIAVI MAP300 PCT - Full 12-Channel IL/RL Test")

    mock_server = None
    if simulation:
        print("[SIM] Starting Mock VIAVI MAP300 server...")
        mock_server = MockViaviServer(
            host=ip, chassis_port=8100, pct_port=8300 + slot
        )
        mock_server.start()
        print(f"[SIM] Mock server running at {ip}:8100 (chassis) & {ip}:{8300+slot} (PCT)")

    driver = None
    try:
        # --- Step 1: Create & Connect ---
        print_section("Step 1: Connecting to VIAVI MAP300")
        driver = create_driver("viavi", ip_address=ip, slot=slot)
        print(f"  Driver: {driver}")

        success = driver.connect()
        if not success:
            print("  [ERROR] Connection failed!")
            return
        print(f"  Connection: OK  (state={driver._state.value})")

        # --- Step 2: Initialize ---
        print_section("Step 2: Initializing Device")
        success = driver.initialize()
        print(f"  Initialize: {'OK' if success else 'FAILED'}")

        # --- Step 3: Configure ORL ---
        print_section("Step 3: Configuring ORL Window")
        driver.configure_orl(method=2, origin=1, a_offset=-0.5, b_offset=0.5)
        print("  ORL: method=Discrete, origin=DUT_Start, A=-0.5m, B=0.5m")

        # --- Step 4: Run Full Test ---
        print_section("Step 4: Running Full Test (Reference + Measurement)")
        wavelengths = [1310.0, 1550.0]
        channels = list(range(1, 13))  # 12 channels
        print(f"  Wavelengths: {wavelengths} nm")
        print(f"  Channels: 1-{len(channels)} ({len(channels)} total)")
        print(f"  Reference: Override (IL=0.1dB, Length=3m)")
        print()

        start_time = time.time()
        results = driver.run_full_test(
            wavelengths=wavelengths,
            channels=channels,
            do_reference=True,
            override_reference=True,
            il_value=0.1,
            length_value=3.0,
        )
        elapsed = time.time() - start_time

        # --- Step 5: Display Results ---
        print_section(f"Step 5: Results ({len(results)} measurements, {elapsed:.2f}s)")
        print()
        print(format_results_table(results))

        # --- Step 6: Summary Statistics ---
        print_section("Step 6: Summary Statistics")
        total = len(results)
        passed = sum(
            1 for r in results
            if r.insertion_loss < 0.5 and r.return_loss > 50.0
        )
        failed = total - passed

        avg_il = sum(r.insertion_loss for r in results) / total if total else 0
        avg_rl = sum(r.return_loss for r in results) / total if total else 0
        max_il = max(r.insertion_loss for r in results) if results else 0
        min_rl = min(r.return_loss for r in results) if results else 0

        print(f"  Total Measurements : {total}")
        print(f"  PASS               : {passed} ({passed/total*100:.1f}%)")
        print(f"  FAIL               : {failed} ({failed/total*100:.1f}%)")
        print(f"  Avg IL             : {avg_il:.4f} dB")
        print(f"  Avg RL             : {avg_rl:.2f} dB")
        print(f"  Max IL (worst)     : {max_il:.4f} dB  {'[OK]' if max_il < 0.5 else '[EXCEED]'}")
        print(f"  Min RL (worst)     : {min_rl:.2f} dB  {'[OK]' if min_rl > 50.0 else '[EXCEED]'}")

        # --- Step 7: Data Formatting for Systems ---
        print_section("Step 7: Data Formatting")

        mims_data = driver.format_results_for_mims(results)
        mas_data = driver.format_results_for_mas(results)
        print(f"  MIMS format: {len(mims_data)} records ready")
        print(f"  MAS  format: {len(mas_data)} records ready")
        print(f"  MIMS sample: {json.dumps(mims_data[0], indent=4)}")

        # --- Step 8: Export ---
        print_section("Step 8: Export Results")
        output_dir = os.path.join(os.path.dirname(__file__), "test_output")
        os.makedirs(output_dir, exist_ok=True)

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

        csv_path = export_results_csv(
            results, os.path.join(output_dir, f"viavi_results_{timestamp}.csv")
        )
        print(f"  CSV exported: {csv_path}")

        json_path = export_results_json(
            results, os.path.join(output_dir, f"viavi_results_{timestamp}.json")
        )
        print(f"  JSON exported: {json_path}")

    except Exception as e:
        print(f"\n  [ERROR] {type(e).__name__}: {e}")
        import traceback
        traceback.print_exc()

    finally:
        # --- Cleanup ---
        if driver:
            driver.disconnect()
            print(f"\n  Disconnected. Final state: {driver._state.value}")
        if mock_server:
            mock_server.stop()
            print("  [SIM] Mock server stopped.")


def demo_viavi_quick_test(ip: str, slot: int, simulation: bool = True):
    """
    Demo 2: VIAVI Quick Test - Single wavelength, fewer channels.

    Shows a simpler test scenario for quick validation.
    """
    print_banner("Demo 2: VIAVI Quick Test (1550nm, 4 Channels)")

    mock_server = None
    if simulation:
        mock_server = MockViaviServer(
            host=ip, chassis_port=8100, pct_port=8300 + slot
        )
        mock_server.start()

    driver = None
    try:
        driver = create_driver("viavi", ip_address=ip, slot=slot)
        driver.connect()
        driver.initialize()

        results = driver.run_full_test(
            wavelengths=[1550.0],
            channels=[1, 2, 3, 4],
            do_reference=True,
            override_reference=True,
            il_value=0.1,
            length_value=3.0,
        )

        print()
        print(format_results_table(results))
        print(f"\n  Quick test done: {len(results)} results collected.")

    except Exception as e:
        print(f"\n  [ERROR] {e}")
    finally:
        if driver:
            driver.disconnect()
        if mock_server:
            mock_server.stop()


def demo_santec_test(simulation: bool = True):
    """
    Demo 3: Santec Equipment Test (Placeholder / Skeleton).

    Demonstrates the Santec driver interface. Since Santec protocol
    documentation is pending, this runs with placeholder logic.
    """
    print_banner("Demo 3: Santec Equipment Test (Skeleton)")

    print("  NOTE: Santec driver is a SKELETON implementation.")
    print("  Real measurements require protocol documentation.")
    print("  This demo shows the unified interface working with placeholder data.")
    print()

    # Santec driver doesn't need mock server (uses internal placeholders)
    # We need to bypass the actual TCP connection for the skeleton demo
    driver = SantecDriver(ip_address="192.168.1.100", port=5000)

    # Manually set state to simulate connected (since no real device)
    driver._state = ConnectionState.CONNECTED
    driver.logger.disabled = True  # Suppress placeholder warnings in demo output

    try:
        driver.initialize()

        driver.configure_wavelengths([1310.0, 1550.0])
        driver.configure_channels([1, 2])

        driver.take_reference()
        driver.take_measurement()
        results = driver.get_results()

        print(f"  Santec driver returned {len(results)} placeholder results:")
        for r in results:
            print(
                f"    Ch {r.channel} @ {r.wavelength:.0f}nm: "
                f"IL={r.insertion_loss:.4f}dB  RL={r.return_loss:.4f}dB "
                f"(placeholder)"
            )

        print("\n  The Santec driver uses the SAME interface as VIAVI:")
        print("    - create_driver('santec', ...)")
        print("    - driver.run_full_test(wavelengths, channels, ...)")
        print("    - driver.format_results_for_mims(results)")
        print("  Once protocol docs arrive, only the internal methods change.")

    finally:
        driver._state = ConnectionState.DISCONNECTED


def demo_factory_pattern():
    """
    Demo 4: Driver Factory Pattern.

    Shows how DriverFactory enables seamless equipment switching.
    """
    print_banner("Demo 4: Driver Factory - Unified Equipment Management")

    print(f"\n  Supported equipment types: {DriverFactory.supported_types()}")
    print()

    # Demonstrate creating drivers via factory
    configs = [
        ("viavi",  {"ip_address": "10.14.132.194", "slot": 1}),
        ("viavi",  {"ip_address": "10.14.132.195", "slot": 2}),
        ("santec", {"ip_address": "192.168.1.100", "port": 5000}),
    ]

    print("  Creating drivers via DriverFactory:")
    for eq_type, kwargs in configs:
        driver = create_driver(eq_type, **kwargs)
        print(f"    {driver}")

    print("\n  All drivers share the same interface (BaseEquipmentDriver):")
    print("    .connect() / .disconnect() / .reconnect()")
    print("    .initialize()")
    print("    .configure_wavelengths() / .configure_channels()")
    print("    .take_reference() / .take_measurement()")
    print("    .get_results()")
    print("    .run_full_test()  [template method - orchestrates all above]")
    print("    .format_results_for_mims() / .format_results_for_mas()")


def demo_multi_device_batch():
    """
    Demo 5: Multi-Device Batch Test.

    Simulates testing across multiple VIAVI devices sequentially,
    as would happen in the Guad factory with 9 VIAVI units.
    """
    print_banner("Demo 5: Multi-Device Batch Test (3 VIAVI Units)")

    # Simulate 3 VIAVI devices on different slots
    devices = [
        {"ip": "127.0.0.1", "slot": 1, "chassis_port": 8100, "pct_port": 8301},
        {"ip": "127.0.0.1", "slot": 2, "chassis_port": 8110, "pct_port": 8312},
        {"ip": "127.0.0.1", "slot": 3, "chassis_port": 8120, "pct_port": 8323},
    ]

    all_results = {}

    for i, dev in enumerate(devices, 1):
        print_section(f"Device {i}: {dev['ip']} slot={dev['slot']}")

        # Start mock server for this device
        mock = MockViaviServer(
            host=dev["ip"],
            chassis_port=dev["chassis_port"],
            pct_port=dev["pct_port"],
        )
        mock.start()

        # Override port calculation for demo (slots on different ports)
        driver = ViaviPCTDriver(ip_address=dev["ip"], slot=dev["slot"])
        driver.CHASSIS_PORT = dev["chassis_port"]
        driver.config.port = dev["pct_port"]

        try:
            driver.connect()
            driver.initialize()

            results = driver.run_full_test(
                wavelengths=[1310.0, 1550.0],
                channels=[1, 2, 3, 4],
                do_reference=True,
                override_reference=True,
                il_value=0.1,
                length_value=3.0,
            )

            all_results[f"Device_{i}"] = results

            passed = sum(
                1 for r in results
                if r.insertion_loss < 0.5 and r.return_loss > 50.0
            )
            print(f"  Results: {len(results)} measurements, {passed}/{len(results)} PASS")

        except Exception as e:
            print(f"  [ERROR] {e}")
        finally:
            driver.disconnect()
            mock.stop()

    # Summary across all devices
    print_section("Batch Summary")
    total_all = sum(len(r) for r in all_results.values())
    pass_all = sum(
        sum(1 for r in results if r.insertion_loss < 0.5 and r.return_loss > 50.0)
        for results in all_results.values()
    )
    print(f"  Devices tested : {len(all_results)}")
    print(f"  Total results  : {total_all}")
    print(f"  Overall PASS   : {pass_all}/{total_all} ({pass_all/total_all*100:.1f}%)")


# =============================================================================
# Main Entry Point
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="VIAVI & Santec IL/RL Test Equipment - Integration Demo",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python test_demo.py                        Run all demos (simulation)
  python test_demo.py --demo 1               Run Demo 1 only
  python test_demo.py --live 10.14.132.194   Connect to real VIAVI device
  python test_demo.py --demo 1 2 3 4 5       Run specific demos
        """,
    )
    parser.add_argument(
        "--live", type=str, default=None, metavar="IP",
        help="Connect to real VIAVI device at this IP (default: simulation mode)",
    )
    parser.add_argument(
        "--slot", type=int, default=1,
        help="PCT module slot number (default: 1)",
    )
    parser.add_argument(
        "--demo", nargs="+", type=int, default=None,
        help="Run specific demo(s) by number: 1-5 (default: all)",
    )
    parser.add_argument(
        "--verbose", "-v", action="store_true",
        help="Enable verbose driver logging",
    )

    args = parser.parse_args()

    # Configure logging
    log_level = logging.DEBUG if args.verbose else logging.WARNING
    logging.basicConfig(
        level=log_level,
        format="%(asctime)s [%(name)s] %(levelname)s: %(message)s",
    )

    simulation = args.live is None
    ip = args.live or "127.0.0.1"
    slot = args.slot
    demos_to_run = args.demo or [1, 2, 3, 4, 5]

    # Header
    print()
    print("*" * 70)
    print("*  VIAVI & Santec IL/RL Test Equipment - Integration Demo")
    print(f"*  Mode: {'SIMULATION (Mock Server)' if simulation else f'LIVE ({ip})'}")
    print(f"*  Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"*  Demos: {demos_to_run}")
    print("*" * 70)

    # Run selected demos
    if 1 in demos_to_run:
        demo_viavi_full_test(ip, slot, simulation)

    if 2 in demos_to_run:
        demo_viavi_quick_test(ip, slot, simulation)

    if 3 in demos_to_run:
        demo_santec_test(simulation)

    if 4 in demos_to_run:
        demo_factory_pattern()

    if 5 in demos_to_run:
        demo_multi_device_batch()

    # Footer
    print_banner("All Demos Complete")
    print(f"  Output files saved to: {os.path.join(os.path.dirname(__file__), 'test_output')}")
    print()


if __name__ == "__main__":
    main()

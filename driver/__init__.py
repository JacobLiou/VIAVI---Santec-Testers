"""
Equipment Driver Package for VIAVI & Santec IL/RL Testers.

Requires Python 3.9+.

This package provides a unified driver framework for integrating
VIAVI MAP300 PCT and Santec IL/RL test equipment into MIMS/Mlight/MAS systems.

Modules:
    base_driver     - Abstract base class and data models
    viavi_driver    - VIAVI MAP300 PCT concrete driver
    santec_driver   - Santec concrete driver (skeleton)
    driver_factory  - Factory pattern for driver creation

Quick Start:
    from driver import create_driver

    # VIAVI
    driver = create_driver("viavi", ip_address="10.14.132.194", slot=1)

    # Santec
    driver = create_driver("santec", ip_address="192.168.1.100", port=5000)
"""

from base_driver import (
    BaseEquipmentDriver,
    ConnectionConfig,
    ConnectionState,
    DeviceInfo,
    MeasurementMode,
    MeasurementResult,
    MeasurementState,
)
from viavi_driver import ViaviPCTDriver
from santec_driver import SantecDriver
from driver_factory import DriverFactory, create_driver

__all__ = [
    "BaseEquipmentDriver",
    "ConnectionConfig",
    "ConnectionState",
    "DeviceInfo",
    "MeasurementMode",
    "MeasurementResult",
    "MeasurementState",
    "ViaviPCTDriver",
    "SantecDriver",
    "DriverFactory",
    "create_driver",
]

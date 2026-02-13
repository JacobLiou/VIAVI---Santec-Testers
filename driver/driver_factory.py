"""
Driver Factory - Factory pattern for creating equipment driver instances.

Provides a unified entry point for creating VIAVI or Santec drivers
without the caller needing to know the specific implementation details.

Author: Menghui
Date: 2026-02-13
Project: VIAVI & Santec Equipment Integration into MIMS/Mlight/MAS
"""

from __future__ import annotations

from base_driver import BaseEquipmentDriver
from viavi_driver import ViaviPCTDriver
from santec_driver import SantecDriver


class DriverFactory:
    """
    Factory class for creating equipment driver instances.

    Supports:
    - VIAVI MAP300 PCT
    - Santec (skeleton - pending protocol documentation)

    Usage:
        driver = DriverFactory.create("viavi", ip="10.14.132.194", slot=1)
        driver = DriverFactory.create("santec", ip="192.168.1.100", port=5000)
    """

    # Registry of supported equipment types
    _REGISTRY = {
        "viavi": ViaviPCTDriver,
        "viavi_pct": ViaviPCTDriver,
        "map300": ViaviPCTDriver,
        "santec": SantecDriver,
    }

    @classmethod
    def create(cls, equipment_type: str, **kwargs) -> BaseEquipmentDriver:
        """
        Create a driver instance for the specified equipment type.

        Args:
            equipment_type: Type of equipment ("viavi", "santec", etc.).
            **kwargs: Constructor arguments for the specific driver.

        Returns:
            An instance of the appropriate driver.

        Raises:
            ValueError: If the equipment type is not supported.

        Examples:
            # VIAVI MAP300 PCT
            driver = DriverFactory.create(
                "viavi",
                ip_address="10.14.132.194",
                slot=1,
                launch_port=1,
            )

            # Santec
            driver = DriverFactory.create(
                "santec",
                ip_address="192.168.1.100",
                port=5000,
            )
        """
        key = equipment_type.lower().strip()
        if key not in cls._REGISTRY:
            supported = ", ".join(sorted(cls._REGISTRY.keys()))
            raise ValueError(
                f"Unsupported equipment type: '{equipment_type}'. "
                f"Supported types: {supported}"
            )

        driver_class = cls._REGISTRY[key]
        return driver_class(**kwargs)

    @classmethod
    def supported_types(cls) -> list[str]:
        """Return list of supported equipment type identifiers."""
        return sorted(cls._REGISTRY.keys())

    @classmethod
    def register(cls, type_name: str, driver_class: type):
        """
        Register a new equipment driver type.

        Args:
            type_name: Identifier for the equipment type.
            driver_class: The driver class (must extend BaseEquipmentDriver).
        """
        if not issubclass(driver_class, BaseEquipmentDriver):
            raise TypeError(
                f"{driver_class.__name__} must inherit from BaseEquipmentDriver"
            )
        cls._REGISTRY[type_name.lower().strip()] = driver_class


# ---------------------------------------------------------------------------
# Convenience function
# ---------------------------------------------------------------------------

def create_driver(equipment_type: str, **kwargs) -> BaseEquipmentDriver:
    """
    Convenience function for creating a driver.

    Same as DriverFactory.create() but as a standalone function.
    """
    return DriverFactory.create(equipment_type, **kwargs)

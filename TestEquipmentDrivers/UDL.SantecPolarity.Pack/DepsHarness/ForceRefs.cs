using System;

// Forces MSBuild to copy Santec.Hardware transitive deps into bin for packaging.
namespace DepsHarness
{
    public static class ForceRefs
    {
        public static Type HardwareMarker()
        {
            return typeof(Santec.Hardware.Devices.Polarity.RDPFactory);
        }
    }
}

using Santec.Hardware.Devices.ReturnLoss;
using Santec.Hardware.Devices.ReturnLoss.Trace;
using Santec.Hardware.Services;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;

namespace RLMTraceSample
{
    /// <summary>
    /// This program provides sample code of obtaining an OTDR trace via the Santec Hardware library.
    /// </summary>
    class Program
    {
        static async Task Main(string[] args)
        {
            List<IRLM> detectedRLMs;
            IRLM rlm;

            string input;
            IPAddress ipAddress;
            bool success;

            TraceDiagnostics trace;

            IHardwareDetectionService hardwareDetectionService = new HardwareDetectionService();

            // Finding the RLM
            //Console.WriteLine("Finding USB RLM...");

            //// Step 1. Find all the USB OPMs
            //detectedRLMs = await hardwareDetectionService.DetectHardwareAsync<IRLM>(EConnectionTypesToDetect.USB);

            //if (!detectedRLMs.Any())
            //{
            //    throw new Exception("Could not find RLM.");
            //}

            //// Step 2. Select the first OPM to use for the sample
            //rlm = detectedRLMs.First();

            //Console.WriteLine("RLM Found. Initializing...");

            // Step 1. Get the IP address
            Console.WriteLine("Enter the IP Address of the RLM:");

            input = Console.ReadLine();

            if (!IPAddress.TryParse(input, out ipAddress))
            {
                throw new Exception("Invalid IP address.");
            }

            // Step 2. Find the RLM
            (success, rlm) = await hardwareDetectionService.TryDetectIPDeviceAsync<IRLM>(ipAddress);

            if (!success)
            {
                throw new Exception("Could not find RLM.");
            }

            // Step 3. Initialize the OPM
            await rlm.InitializeAsync();

            Console.WriteLine("RLM initialized.");

            // Step 4. Set the RLM to Remote mode.
            await rlm.SetInteractionModeAsync(Santec.Hardware.Devices.General.EInteractionMode.Remote);

            Console.WriteLine("RLM set to remote mode.");

            // Obtain an OTDR trace
            Console.WriteLine("Obtaining OTDR trace...");

            // Step 1. Run an RL reading
            await rlm.ReadRLAsync(rlm.Wavelengths.First());

            Console.WriteLine("Retrieving OTDR trace...");

            // Step 2. Retrieve the OTDR trace from the previous RL reading
            trace = await rlm.GetTraceDiagnosticsAsync();

            Console.WriteLine("OTDR trace retrieved.");
            Console.WriteLine();

            // Use the trace
            // For the sample we print out all of the trace information
            if (trace.DiagnosticInfo.RLMType == ERLMType.SingleMode)
            {
                Console.WriteLine("RLM type: Single mode");
            }
            else
            {
                Console.WriteLine("RLM type: Multimode");
            }

            Console.WriteLine($"Dither: {trace.DiagnosticInfo.Dither}");
            Console.WriteLine($"Dither Mode: {trace.DiagnosticInfo.DitherMode}");

            Console.WriteLine($"APD Bias: {trace.DiagnosticInfo.APDBias}");

            Console.WriteLine($"Number of Phases: {trace.DiagnosticInfo.NumberOfPhases}");
            Console.WriteLine($"Number of Iterations: {trace.DiagnosticInfo.NumberOfIterations}");
            Console.WriteLine($"Start Phase: {trace.DiagnosticInfo.StartPhase}");

            Console.WriteLine($"Off Pre Delay: {trace.DiagnosticInfo.OffPreDelay}");
            Console.WriteLine($"Off Sample: {trace.DiagnosticInfo.OffSample}");
            Console.WriteLine($"Off Post Delay: {trace.DiagnosticInfo.OffPostDelay}");

            Console.WriteLine($"Acquisition Pre Delay: {trace.DiagnosticInfo.AcquisitionPreDelay}");
            Console.WriteLine($"Acquisition Sample: {trace.DiagnosticInfo.AcquisitionSample}");
            Console.WriteLine($"Acquisition Post Delay: {trace.DiagnosticInfo.AcquisitionPostDelay}");

            Console.WriteLine($"Laser Pulse Width: {trace.DiagnosticInfo.LaserPulseWidth}");

            Console.WriteLine($"Peak Detection Configuration: {trace.DiagnosticInfo.PeakDetectionConfiguration}");

            Console.WriteLine($"Delta D: {trace.DiagnosticInfo.DeltaD}");
            Console.WriteLine($"Delta T: {trace.DiagnosticInfo.DeltaT}");

            Console.WriteLine($"Start of Fiber Index: {trace.DiagnosticInfo.StartOfFiberIndex}");
            Console.WriteLine($"End of Fiber Bottom Index: {trace.DiagnosticInfo.EndOfFiberBottomIndex}");
            Console.WriteLine($"End of Fiber Top Index: {trace.DiagnosticInfo.EndOfFiberTopIndex}");

            Console.WriteLine($"Referenced End of Fiber Index: {trace.DiagnosticInfo.ReferencedEndOfFiberIndex}");
            Console.WriteLine($"Zero Position Index: {trace.DiagnosticInfo.ZeroPositionIndex}");

            Console.WriteLine($"Internal Fiber Length: {trace.DiagnosticInfo.InternalFiberLength}");
            Console.WriteLine($"Measured Fiber Length: {trace.DiagnosticInfo.MeasuredFiberLength}");

            Console.WriteLine($"Injection Level Start Index: {trace.DiagnosticInfo.InjectionLevelStartIndex}");
            Console.WriteLine($"Injection Level End Index: {trace.DiagnosticInfo.InjectionLevelEndIndex}");
            Console.WriteLine($"Injection Level: {trace.DiagnosticInfo.InjectionLevel}");

            Console.WriteLine($"MTJ IL: {trace.DiagnosticInfo.MTJil}");
            Console.WriteLine($"Start MTJ Length: {trace.DiagnosticInfo.StartMTJLength}");
            Console.WriteLine($"End MTJ Length: {trace.DiagnosticInfo.EndMTJLength}");

            Console.WriteLine($"RLa Reference Index: {trace.DiagnosticInfo.RLaReferenceIndex}");
            Console.WriteLine($"RLa Index: {trace.DiagnosticInfo.RLaIndex}");
            Console.WriteLine($"RLa Peak Height: {trace.DiagnosticInfo.RLaPeakHeight}");
            Console.WriteLine($"RLa H Value: {trace.DiagnosticInfo.RLaH}");
            Console.WriteLine($"RLa: {trace.DiagnosticInfo.RLa}");

            Console.WriteLine($"RLb Reference Index: {trace.DiagnosticInfo.RLbReferenceIndex}");
            Console.WriteLine($"RLb Index: {trace.DiagnosticInfo.RLbIndex}");
            Console.WriteLine($"RLb Peak Height: {trace.DiagnosticInfo.RLbPeakHeight}");
            Console.WriteLine($"RLb H Value: {trace.DiagnosticInfo.RLbH}");
            Console.WriteLine($"RLb: {trace.DiagnosticInfo.RLb}");

            Console.WriteLine($"RL Total Start Index: {trace.DiagnosticInfo.RLtotalStartIndex}");
            Console.WriteLine($"RL Total End Index: {trace.DiagnosticInfo.RLtotalEndIndex}");
            Console.WriteLine($"RL Total: {trace.DiagnosticInfo.RLtotal}");

            Console.WriteLine($"Base IOR: {trace.DiagnosticInfo.BaseIOR}");

            Console.WriteLine($"Ref Peak Height Start: {trace.DiagnosticInfo.RefPeakHeightStart}");

            Console.WriteLine($"DUT IL: {trace.DiagnosticInfo.DUTil}");

            Console.WriteLine($"End of Fiber Peak Index: {trace.DiagnosticInfo.EndOfFiberPeakIndex}");
            Console.WriteLine($"End of Fiber Peak Height: {trace.DiagnosticInfo.EndOfFiberPeakHeight}");

            Console.WriteLine();
            Console.WriteLine();
            Console.WriteLine($"Trace Points: {trace.TraceData.Count()}");
            Console.WriteLine($"Trace: {string.Join(", ", trace.TraceData)}");

            Console.WriteLine();
            Console.WriteLine($"Trace Points: {trace.TraceData.Count()}");

            // Cleanup
            // Step 1. Uninitialize the RLM.
            rlm.Uninitialize();

            Console.WriteLine("Press any key to exit...");

            Console.ReadKey();
        }
    }
}

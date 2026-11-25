using Santec.Hardware.Devices.General;
using Santec.Hardware.Devices.Polarity;
using Santec.Hardware.Devices.ReturnLoss;
using Santec.Hardware.Exceptions;
using Santec.Hardware.Services;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace RDPSample
{
    class Program
    {
        static async Task Main(string[] args)
        {
            IRDPFactory rdpFactory = new RDPFactory();
            IHardwareDetectionService hardwareDetectionService = new HardwareDetectionService();
            List<IRLM> detectedRLMs;
            IRLM rlm = null;
            IRDP rdp;
            DeviceDetector detector;

            int wavelength;
            double il;

            IReadOnlyList<int?> polarityMapping;

            bool fiberDetected;

            // Creating an RD-P
            Console.WriteLine("Creating the RD-P (RD-SP)...");

            // Step 1. Check that an RD-P camera is connected to the computer.
            if (!rdpFactory.CheckForCamera())
            {
                throw new Exception("Could not find RD-P (RD-SP).");
            }

            // Step 2. Find all USB RLMs
            detectedRLMs = await hardwareDetectionService.DetectHardwareAsync<IRLM>(EConnectionTypesToDetect.USB);

            // Step 3. Find the first RLM that meets the RD-P hardware requirements
            foreach(IRLM detectedRLM in detectedRLMs)
            {
                //RLMs must be initialized
                await detectedRLM.InitializeAsync();

                if (rdpFactory.CheckIfRLMMeetsHardwareRequirements(detectedRLM))
                {
                    //found an RLM that meets the hardware requirements
                    rlm = detectedRLM;
                    break;
                }
                else
                {
                    //uninitialize the RLM since it does not meet the requirements
                    detectedRLM.Uninitialize();
                }
            }

            if (rlm == null)
            {
                throw new Exception("Could not find RLM that meets the RD-P (RD-SP) hardware requirements.");
            }

            // Step 4. Create the RD-P (RD-SP)
            rdp = rdpFactory.CreateRDP(rlm);
            Console.WriteLine("Created " + rdp.Name);

            // Step 5. Initialize the RD-P (RD-SP)
            await rdp.InitializeAsync();

            // Basic Usage- 12 channel MPO
            Console.WriteLine("Running basic polarity mapping...");

            // Step 1. Set the RD-P mapping mode, mapping sensitivity, and loss compensation
            rdp.SetMappingMode(EMappingMode.MPO);

            rdp.SetMappingSensitivity(EMappingSensitivity.Connector);

            await rdp.SetLossCompensationAsync(0);

            //Step 2. Perform the polarity mapping
            try
            {
                polarityMapping = await rdp.MapPolarityAsync(12, new List<int?> { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 });

                //show the polarity mapping
                Console.WriteLine("Basic polarity mapping: " + string.Join(",", polarityMapping));
            }
            catch(PolarityMappingFailedException ex)
            {
                Console.WriteLine("Basic polarity mapping failed. Saving image...");

                //save an image of the failed polarity mapping
                ex.PolarityMappingImage.SaveImage("Basic Polarity Mapping Failed.png");

                Console.WriteLine("Image saved.");
            }

            // Advanced Usage: Segmented Polarity Mapping- 12 channel MPO
            Console.WriteLine("Running segmented polarity mapping...");

            // Step 1. Set the RD-P mapping mode, mapping sensitivity, and loss compensation
            rdp.SetMappingMode(EMappingMode.MPO);

            rdp.SetMappingSensitivity(EMappingSensitivity.Connector);

            await rdp.SetLossCompensationAsync(0);

            // Step 2. Start the polarity mapping operation
            await rdp.StartMappingPolarityAsync(12);

            // Step 3. Setup the RLM for the test
            await rlm.SetInteractionModeAsync(EInteractionMode.Remote);

            wavelength = rlm.Wavelengths.First(rlmWavelength => rlmWavelength == 1310 || rlmWavelength == 1300);

            await rlm.TurnOnLaserAsync(wavelength);

            // Step 4. Switch to the correct optical path for each fiber and collect the mapping data, measuring IL if we have an RD-SP
            await rlm.ChangeOutputChannelAsync(1);

            detector = rlm.Detectors.FirstOrDefault();

            for (int i = 0; i < 12; i++)
            {
                Console.WriteLine($"Fiber {i + 1}:");

                //If we have an RD-SP, measure IL
                if(detector.IsRDSP)
                {
                    il = await rlm.ReadILAsync(detector.Index, wavelength);

                    Console.WriteLine($"IL @ {wavelength}nm: {il}");

                    // Reading IL turns off the laser so we must turn it back on
                    await rlm.TurnOnLaserAsync(wavelength);
                }

                //in this case, we are just running through the channels in reverse
                await rlm.ChangeSwitchChannelAsync(1, 12 - i);

                // When we collect the mapping data, the method returns whether a fiber was detected
                fiberDetected = await rdp.CollectOutputMappingDataAsync(i + 1);

                if (fiberDetected)
                {
                    Console.WriteLine($"Fiber {i + 1} detected.");
                }
                else
                {
                    Console.WriteLine($"Fiber {i + 1} not detected.");
                }
            }

            // Step 5. Cleanup from the test
            await rlm.TurnOffLaserAsync();

            await rlm.SetInteractionModeAsync(EInteractionMode.Local);

            // Step 6. Process the collected mapping data into a mapping
            try
            {
                polarityMapping = rdp.ProcessMappingData(new List<int?> { 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 });

                //show the polarity mapping
                Console.WriteLine("Advanced polarity mapping: " + string.Join(",", polarityMapping));
            }
            catch (PolarityMappingFailedException ex)
            {
                Console.WriteLine("Advanced polarity mapping failed. Saving image...");

                //save an image of the failed polarity mapping
                ex.PolarityMappingImage.SaveImage("Advanced Polarity Mapping Failed.png");

                Console.WriteLine("Image saved.");
            }

            // Step 7. End the polarity mapping operation
            await rdp.EndMappingPolarityAsync();

            // Cleanup
            // Step 1. Uninitialize the RD-P.
            rdp.Uninitialize();

            Console.WriteLine("Press any key to exit...");

            Console.ReadKey();
        }
    }
}

using Santec.Hardware.Devices.ReturnLoss;
using Santec.Hardware.Devices.Switching;
using Santec.Hardware.Services;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;

namespace RLMSample
{
    class Program
    {
        /// <summary>
        /// This program provides sample code of operating an RLM via the Santec Hardware library. It should be noted that it does not cover the full range of RLM capabilities.
        /// </summary>
        static async Task Main(string[] args)
        {
            List<IRLM> detectedRLMs;
            IRLM rlm;
            List<List<DeviceSwitchAndChannelPair>> allChannelCombinations = null;

            IHardwareDetectionService hardwareDetectionService = new HardwareDetectionService();

            double reading;
            RLReading rlReading;

            // Finding the RLM
            Console.WriteLine("Finding USB RLM...");

            // Step 1. Find all the USB RLMs
            detectedRLMs = await hardwareDetectionService.DetectHardwareAsync<IRLM>(EConnectionTypesToDetect.USB);

            if (!detectedRLMs.Any())
            {
                throw new Exception("Could not find RLM.");
            }

            // Step 2. Select the first RLM to use for the sample
            rlm = detectedRLMs.First();

            Console.WriteLine("RLM Found. Initializing...");

            // Step 3. Initialize the RLM
            await rlm.InitializeAsync();

            Console.WriteLine("RLM initialized.");

            // Step 4. Set the RLM to Remote mode.
            await rlm.SetInteractionModeAsync(Santec.Hardware.Devices.General.EInteractionMode.Remote);

            Console.WriteLine("RLM set to remote mode.");

            // Configure the return loss settings
            Console.WriteLine("Configuring RLM return loss settings.");

            // Step 1. Set the RL mode. For the sample we will use fast mode.
            await rlm.SetRLModeAsync(ERLMode.Fast);

            // Step 2. Set the RL length bin. For the sample we assume the DUT is less than 100m and use the corresponding bin.
            await rlm.SetLengthModeAsync(ELengthMode.LessThan100m);

            // Determine the switch settings
            // Create a list of all possible switch combinations
            if (rlm.Switches.Count() > 0)
            {
                allChannelCombinations = Enumerable.Range(1, rlm.Switches.First().NumberOfChannels).Select(channel => new List<DeviceSwitchAndChannelPair>() { new DeviceSwitchAndChannelPair(rlm.Switches.First().Index, channel) }).ToList();

                for(int i = 1; i < rlm.Switches.Count(); i++)
                {
                    allChannelCombinations = allChannelCombinations.SelectMany(combination => Enumerable.Range(1, rlm.Switches[i].NumberOfChannels).Select(
                        channel => new List<DeviceSwitchAndChannelPair>() { new DeviceSwitchAndChannelPair(rlm.Switches[i].Index, channel) }).Select(
                        switchAndChannelPair => (new List<DeviceSwitchAndChannelPair>(combination)).Concat(switchAndChannelPair).ToList()
                        )).ToList();
                }

                Console.WriteLine("Determined switch channel combinations.");
            }
            else
            {
                Console.WriteLine("RLM has no switches.");
            }

            // Reference the RLM
            Console.WriteLine("Press any key to reference IL and RL...");

            Console.ReadKey();
            Console.WriteLine();

            // Use the RLM for IL
            if (rlm.NumberOfDetectors < 1)
            {
                Console.WriteLine("RLM has no detectors. Skipping IL.");

                Console.WriteLine("Referencing RL for all channels and wavelengths...");
            }
            else
            {
                Console.WriteLine("Referencing IL for all channels and wavelengths for detector 1 and RL for all channels and wavelengths...");
            }

            if (rlm.Switches.Count() > 0)
            {
                foreach (List<DeviceSwitchAndChannelPair> channelCombination in allChannelCombinations)
                {
                    // Step 1. Switch to the appropriate switch channels
                    await rlm.ChangeMultipleSwitchesChannelsAsync(channelCombination);

                    foreach (DeviceSwitchAndChannelPair switchAndChannelPair in channelCombination)
                    {
                        if (switchAndChannelPair.DeviceSwitchIndex == 0)
                        {
                            Console.WriteLine($"Switched to output {switchAndChannelPair.Channel}.");
                        }
                        else
                        {
                            Console.WriteLine($"Switched switch {switchAndChannelPair.DeviceSwitchIndex} to channel {switchAndChannelPair.Channel}.");
                        }
                    }

                    // Step 2. Reference the IL at each wavelength
                    if (rlm.NumberOfDetectors >= 1)
                    {
                        foreach (int wavelength in rlm.Wavelengths)
                        {
                            reading = await rlm.ReferenceILAsync(1, wavelength);

                            Console.WriteLine($"Referenced IL @ {wavelength}nm: {reading}");
                        }
                    }

                    // Step 3. Reference RL
                    reading = await rlm.ReferenceRLAsync();

                    Console.WriteLine($"Referenced RL: {reading}");
                }
            }
            else
            {
                // Step 1. Reference the IL at each wavelength
                if (rlm.NumberOfDetectors >= 1)
                {
                    foreach (int wavelength in rlm.Wavelengths)
                    {
                        reading = await rlm.ReferenceILAsync(1, wavelength);

                        Console.WriteLine($"Referenced IL @ {wavelength}nm: {reading}");
                    }
                }

                // Step 2. Reference RL
                reading = await rlm.ReferenceRLAsync();

                Console.WriteLine($"Referenced RL: {reading}");
            }

            // Save the IL references if needed, this allows IL Reference to persist across power cycles
            await rlm.SaveILReferencesAsync();

            if (rlm.NumberOfDetectors < 1)
            {
                Console.WriteLine("Done referencing RL.");

                Console.WriteLine("Press any key to measure RL...");
            }
            else
            {
                Console.WriteLine("Done referencing IL for detector 1 and RL.");

                Console.WriteLine("Press any key to measure IL and RL...");
            }

            Console.ReadKey();
            Console.WriteLine();

            //Measure IL
            if (rlm.NumberOfDetectors < 1)
            {
                Console.WriteLine("Measuring RL for all channels and wavelengths...");
            }
            else
            {
                Console.WriteLine("Measuring IL for all channels and wavelengths for detector 1 and RL for all channels and wavelengths...");
            }

            if (rlm.Switches.Count() > 0)
            {
                foreach (List<DeviceSwitchAndChannelPair> channelCombination in allChannelCombinations)
                {
                    // Step 1. Switch to the appropriate switch channels
                    await rlm.ChangeMultipleSwitchesChannelsAsync(channelCombination);

                    foreach (DeviceSwitchAndChannelPair switchAndChannelPair in channelCombination)
                    {
                        if (switchAndChannelPair.DeviceSwitchIndex == 0)
                        {
                            Console.WriteLine($"Switched to output {switchAndChannelPair.Channel}.");
                        }
                        else
                        {
                            Console.WriteLine($"Switched switch {switchAndChannelPair.DeviceSwitchIndex} to channel {switchAndChannelPair.Channel}.");
                        }
                    }

                    foreach (int wavelength in rlm.Wavelengths)
                    {
                        // Step 2. Measure the IL at each wavelength
                        if (rlm.NumberOfDetectors >= 1)
                        {
                            reading = await rlm.ReadILAsync(1, wavelength);

                            Console.WriteLine($"IL @ {wavelength}nm: {reading}");
                        }

                        // Step 3. Measure the RL at each wavelength
                        rlReading = await rlm.ReadRLAsync(wavelength);

                        // Output RLa
                        if (double.IsNaN(rlReading.RLa.Value))
                        {
                            Console.WriteLine($"RLa @ {wavelength}nm: --");
                        }
                        else if (rlReading.RLa.RangeStatus == Santec.Hardware.Devices.General.ERangeStatus.OutOfUpperRange)
                        {
                            Console.WriteLine($"RLa @ {wavelength}nm: >{rlReading.RLa.Value}");
                        }
                        else
                        {
                            Console.WriteLine($"RLa @ {wavelength}nm: {rlReading.RLa.Value}");
                        }

                        // Output RLb
                        if (double.IsNaN(rlReading.RLb.Value))
                        {
                            Console.WriteLine($"RLb @ {wavelength}nm: --");
                        }
                        else if (rlReading.RLb.RangeStatus == Santec.Hardware.Devices.General.ERangeStatus.OutOfUpperRange)
                        {
                            Console.WriteLine($"RLb @ {wavelength}nm: >{rlReading.RLb.Value}");
                        }
                        else
                        {
                            Console.WriteLine($"RLb @ {wavelength}nm: {rlReading.RLb.Value}");
                        }

                        // Output RLtotal
                        if (double.IsNaN(rlReading.RLtotal.Value))
                        {
                            Console.WriteLine($"RLtotal @ {wavelength}nm: --");
                        }
                        else if (rlReading.RLtotal.RangeStatus == Santec.Hardware.Devices.General.ERangeStatus.OutOfUpperRange)
                        {
                            Console.WriteLine($"RLtotal @ {wavelength}nm: >{rlReading.RLtotal.Value}");
                        }
                        else
                        {
                            Console.WriteLine($"RLtotal @ {wavelength}nm: {rlReading.RLtotal.Value}");
                        }

                        // Output Length
                        Console.WriteLine($"Length @ {wavelength}nm: {rlReading.Length}");
                    }
                }
            }
            else
            {
                foreach (int wavelength in rlm.Wavelengths)
                {
                    // Step 2. Measure the IL at each wavelength
                    if (rlm.NumberOfDetectors >= 1)
                    {
                        reading = await rlm.ReadILAsync(1, wavelength);

                        Console.WriteLine($"IL @ {wavelength}nm: {reading}");
                    }

                    // Step 2. Measure the RL at each wavelength
                    rlReading = await rlm.ReadRLAsync(wavelength);

                    // Output RLa
                    if (double.IsNaN(rlReading.RLa.Value))
                    {
                        Console.WriteLine($"RLa @ {wavelength}nm: --");
                    }
                    else if (rlReading.RLa.RangeStatus == Santec.Hardware.Devices.General.ERangeStatus.OutOfUpperRange)
                    {
                        Console.WriteLine($"RLa @ {wavelength}nm: >{rlReading.RLa.Value}");
                    }
                    else
                    {
                        Console.WriteLine($"RLa @ {wavelength}nm: {rlReading.RLa.Value}");
                    }

                    // Output RLb
                    if (double.IsNaN(rlReading.RLb.Value))
                    {
                        Console.WriteLine($"RLb @ {wavelength}nm: --");
                    }
                    else if (rlReading.RLb.RangeStatus == Santec.Hardware.Devices.General.ERangeStatus.OutOfUpperRange)
                    {
                        Console.WriteLine($"RLb @ {wavelength}nm: >{rlReading.RLb.Value}");
                    }
                    else
                    {
                        Console.WriteLine($"RLb @ {wavelength}nm: {rlReading.RLb.Value}");
                    }

                    // Output RLtotal
                    if (double.IsNaN(rlReading.RLtotal.Value))
                    {
                        Console.WriteLine($"RLtotal @ {wavelength}nm: --");
                    }
                    else if (rlReading.RLtotal.RangeStatus == Santec.Hardware.Devices.General.ERangeStatus.OutOfUpperRange)
                    {
                        Console.WriteLine($"RLtotal @ {wavelength}nm: >{rlReading.RLtotal.Value}");
                    }
                    else
                    {
                        Console.WriteLine($"RLtotal @ {wavelength}nm: {rlReading.RLtotal.Value}");
                    }

                    // Output Length
                    Console.WriteLine($"Length @ {wavelength}nm: {rlReading.Length}");
                }
            }

            if (rlm.NumberOfDetectors < 1)
            {
                Console.WriteLine("Done measuring RL.");
            }
            else
            {
                Console.WriteLine("Done measuring IL for detector 1 and RL.");
            }

            // Cleanup
            // Step 1. Uninitialize the RLM.
            rlm.Uninitialize();

            Console.WriteLine("Press any key to exit...");

            Console.ReadKey();
        }
    }
}

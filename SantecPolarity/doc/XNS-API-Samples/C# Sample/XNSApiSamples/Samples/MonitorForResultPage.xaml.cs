using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Net.Http.Json;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace XNSApiSamples.Samples
{
    /// <summary>
    /// This sample shows how to monitor the XNS server for a specific serial number and then retrieve all results related to that serial number.
    ///
    /// It uses the server subscription to receive new results with a polling backup to handle the scenario where a result might have been missed by the server.
    ///
    /// All results should be returned via the subscriber and the polling backup exists as peace of mind that the result will be seen.
    /// </summary>
    ///
    /// <remarks>
    /// Note: This sample code uses the System.Net.Http.Json package (https://www.nuget.org/packages/System.Net.Http.Json).
    ///
    /// The example code uses case sensitive property names to support displaying the JSON within the sample program.
    ///
    /// The example code use JSON ignore conditions to support properly displaying the JSON within the sample program.
    /// Specifically, it is to properly display the empty JSON objects that are returned by some calls.
    /// Empty JSON objects are converted to objects with all properties set to null within the program.
    /// </remarks>

    /// <summary>
    /// Interaction logic for MonitorForResultPage.xaml
    /// </summary>
    public partial class MonitorForResultPage : Page, IAPICallSample
    {
        MainWindow parent;
        JSONProcessor jsonProcessor;
        DateTime pollingStartDate;

        HttpListener resultsListener;
        HttpClient serverConnection;

        System.Threading.Timer pollingTimer;

        string localIPAddress;

        bool monitoringForResult;

        SemaphoreSlim resultSemaphore;

        public MonitorForResultPage(MainWindow parent)
        {
            IPAddress[] iPAddresses;

            InitializeComponent();

            this.parent = parent;

            jsonProcessor = new JSONProcessor();

            pollingStartDate = DateTime.Now;

            //determine our IP address
            iPAddresses = System.Net.Dns.GetHostAddresses(System.Net.Dns.GetHostName());

            foreach (IPAddress ipAddress in iPAddresses)
            {
                if (ipAddress.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                {
                    localIPAddress = ipAddress.ToString();
                    break;
                }
            }

            resultSemaphore = new SemaphoreSlim(1);

            MonitoringForResult = false;
        }

        public string SampleName
        {
            get
            {
                return "Monitor for Result";
            }
        }

        private bool MonitoringForResult
        {
            get
            {
                return monitoringForResult;
            }
            set
            {
                monitoringForResult = value;

                //first polling query will be from the time we started monitoring for results
                pollingStartDate = DateTime.Now;

                //update whether controls are enabled/disabled
                DUTSerialNumberTextBox.IsEnabled = !MonitoringForResult;
                DUTSerialNumberCaseSensitiveCheckBox.IsEnabled = !MonitoringForResult;

                MonitorForResultButton.IsEnabled = !MonitoringForResult;
                CancelButton.IsEnabled = MonitoringForResult;
                StartPollingBackupButton.IsEnabled = MonitoringForResult;
            }
        }

        /// <summary>
        /// Update the API call based on the server address and DUT serial number.
        /// </summary>
        /// <remarks> This method exists solely to support the sample program UI. </remarks>
        public void UpdateAPICall()
        {
            //ex. http://localhost:8083/api/subscribers/create-subscriber
            APICall1TextBlock.Text = "http://" + parent.ServerAddressTextBox.Text + ":8083/api/subscribers/create-subscriber";

            //ex. http://localhost:8083/integeration/results?dutSn=12345678
            APICall2TextBlock.Text = "http://" + parent.ServerAddressTextBox.Text + ":8083/integration/results?dutSn=" + DUTSerialNumberTextBox.Text;

            //ex. http://localhost:8083/integeration/results/filter-results?filterParams={"testDate":{"startDate":"2022-03-24 00:00:00"}}
            APICall3TextBlock.Text = "http://" + parent.ServerAddressTextBox.Text + ":8083/integration/results/filter-results?filterParams={\"testDate\":{\"startDate\":\"" + pollingStartDate.ToString("o") + "\"}}";

            //ex. http://localhost:8083/integeration/results?dutSn=12345678
            APICall4TextBlock.Text = "http://" + parent.ServerAddressTextBox.Text + ":8083/integration/results?dutSn=" + DUTSerialNumberTextBox.Text;
        }

        /// <summary>
        /// Determine whether a serial number matches the target serial number we are monitoring for.
        /// </summary>
        /// <param name="serialNumber"></param>
        /// <returns></returns>
        private bool CheckSerialNumberMatchesTargetSerialNumber(string serialNumber)
        {
            bool caseSensitive = true;
            string targetSerialNumber = string.Empty;

            //use the dispatcher to retrieve UI values since this can be run not on the UI thread
            Dispatcher.Invoke(() =>
            {
                caseSensitive = (bool)DUTSerialNumberCaseSensitiveCheckBox.IsChecked;

                targetSerialNumber = DUTSerialNumberTextBox.Text;
            });

            if (caseSensitive)
            {
                return serialNumber == targetSerialNumber;
            }

            //case insensitive comparison
            return string.Compare(serialNumber, targetSerialNumber, StringComparison.OrdinalIgnoreCase) == 0;
        }

        private async void MonitorForResultButton_Click(object sender, RoutedEventArgs e)
        {
            JsonSerializerOptions serializationOptions;

            MonitoringForResult = true;

            //Update the API calls to update the polling query
            UpdateAPICall();

            //Create the HttpClient to interact with the XNS server
            serverConnection = new HttpClient
            {
                Timeout = TimeSpan.FromSeconds(5)
            };

            //Setup the JSON serializer options
            //PropertyNameCaseInsensitive is set so that properties in the parsed objects do not need to match the JSON property names exactly for letter casing
            serializationOptions = new JsonSerializerOptions()
            {
                PropertyNameCaseInsensitive = true
            };

            //clear the response text- this is for displaying the JSON within the sample program
            JSONResponseRichTextBox.Document.Blocks.Clear();

            Subscriber payload = new Subscriber { URL = "http://" + localIPAddress + ":8556" };

            try
            {
                //asynchronously call the XNS Server to POST the new subscriber
                _ = await serverConnection.PostAsJsonAsync<Subscriber>(APICall1TextBlock.Text, payload);

                _ = MessageBox.Show("Subscribed successfully. Monitoring for result.");

                //run our local httpserver to receive notifications from the XNS Server
                resultsListener = new HttpListener();

                resultsListener.Prefixes.Add("http://*:" + "8556/");

                resultsListener.Start();

                _ = resultsListener.BeginGetContext(NewDUTCombinedMeasurementsCallback, null);
            }
            // With the sample code, "An error occurred while sending the request." usually means the server address is incorrect.
            // In general, a bad request indicates there is a syntax or parameter error with API call.
            catch (HttpRequestException ex)
            {
                _ = MessageBox.Show(ex.Message);

                MonitoringForResult = false;
            }
        }

        /// <summary>
        /// Handle receiving new results from the server.
        /// </summary>
        /// <param name="result"></param>
        private async void NewDUTCombinedMeasurementsCallback(IAsyncResult result)
        {
            HttpListenerContext context;
            JsonSerializerOptions serializationOptions;
            NewResults newResults;
            List<DUTResults> DUTResults;
            string callURL = string.Empty;
            string content;
            string rawJSON;

            //Setup the JSON serializer options
            //PropertyNameCaseInsensitive is set so that properties in the parsed objects do not need to match the JSON property names exactly for letter casing
            serializationOptions = new JsonSerializerOptions()
            {
                PropertyNameCaseInsensitive = true
            };

            //Add the converter for handling deserializing the list of DUT result sets.
            serializationOptions.Converters.Add(new JSONDUTResultsConverter());

            if (resultsListener == null)
            {
                return;
            }

            //We use a semaphore to synchronize checking the results between the subscriber and the polling- This is to avoid the scenario where we retrieve the results through both methods at the same time or while canceling the operation
            await resultSemaphore.WaitAsync();

            //read the results from the server
            try
            {
                context = resultsListener.EndGetContext(result);
            }
            catch (ObjectDisposedException)
            {
                return;
            }

            using (StreamReader stream = new StreamReader(context.Request.InputStream))
            {
                content = stream.ReadToEnd();
            }

            //Check if the results have already been retrieved or the monitoring operation has been canceled
            if (!MonitoringForResult)
            {
                _ = resultSemaphore.Release();
                return;
            }

            try
            {
                //The DUTCombinedMeasurements object contains all of the parsed data according to the classes defined in GetMeasurementsPage.xaml.cs.
                newResults = JsonSerializer.Deserialize<NewResults>(content, serializationOptions);

                //check if the serial number of the new result matches the serial number we are monitoring for
                if (CheckSerialNumberMatchesTargetSerialNumber(newResults.DUTCombinedMeasurements.DUTSerialNumber))
                {
                    Dispatcher.Invoke(() => {
                        _ = MessageBox.Show("Subscriber received result. Retrieving full set of results for DUT from the server.");

                        //Use the dispatcher since we change UI element properties within the monitoringForResultProperty
                        MonitoringForResult = false;

                        //Form the call URL with the correct serial number- this is for the case where the target serial number is not case sensitive. All serial numbers in the server are case sensitive.
                        callURL = "http://" + parent.ServerAddressTextBox.Text + ":8083/integration/results?dutSn=" + newResults.DUTCombinedMeasurements.DUTSerialNumber;
                    });

                    try
                    {
                        //asynchronously call the server get results API and parse the results
                        //The list of DUTResults objects contains all of the parsed data according to the classes defined above.
                        DUTResults = await serverConnection.GetFromJsonAsync<List<DUTResults>>(callURL, serializationOptions);

                        // Serialize the DUT results to JSON text to format and display
                        // This is done solely for displaying the JSON text in the sample program
                        rawJSON = JsonSerializer.Serialize(DUTResults, serializationOptions);

                        //format, colorize, and display the JSON
                        //use the dispatcher since we are not on the UI thread
                        Dispatcher.Invoke(() => {
                            JSONResponseRichTextBox.Document.Blocks.Add(jsonProcessor.FormatAndColorize(rawJSON));
                        });

                        //indicate a success in the response to the server
                        context.Response.StatusCode = (int)HttpStatusCode.OK;
                        context.Response.Close();

                        StopMonitoringForResult();
                    }
                    // With the sample code, "An error occurred while sending the request." usually means the server address is incorrect.
                    // "Response status code does not indicate success: 400 (Bad request)" usually means there are no measurements corresponding to the input serial number
                    // In general, a bad request indicates there is a syntax or parameter error with API call.
                    catch (HttpRequestException ex)
                    {
                        _ = MessageBox.Show(ex.Message);
                    }
                }
                else
                {
                    //indicate a success in the response to the server
                    context.Response.StatusCode = (int)HttpStatusCode.OK;
                    context.Response.Close();
                }
            }
            catch (Exception)
            {
                //indicate an error in the response to the server
                context.Response.StatusCode = (int)HttpStatusCode.InternalServerError;
                context.Response.Close();
            }

            //Listen for a new result if we have not found the result we were monitoring for.
            if (MonitoringForResult)
            {
                _ = resultsListener.BeginGetContext(NewDUTCombinedMeasurementsCallback, null);
            }

            _ = resultSemaphore.Release();
        }

        private void StartPollingBackupButton_Click(object sender, RoutedEventArgs e)
        {
            StartPollingBackupButton.IsEnabled = false;

            //poll the server every 5 seconds for the target serial number
            pollingTimer = new System.Threading.Timer(PollingCallback, null, 0, 5000);

            _ = MessageBox.Show("Polling backup started. Monitoring for result.");
        }

        private async void PollingCallback(object state)
        {
            JsonSerializerOptions serializationOptions;
            List<FilteredDUTInformation> filteredDUTsInformation;
            List<DUTResults> DUTResults;
            FilteredDUTInformation targetDUT;
            string rawJSON;

            string callURL = string.Empty;

            //Setup the JSON serializer options
            //PropertyNameCaseInsensitive is set so that properties in the parsed objects do not need to match the JSON property names exactly for letter casing
            serializationOptions = new JsonSerializerOptions()
            {
                PropertyNameCaseInsensitive = true
            };

            //Add the converter for handling deserializing the list of DUT result sets.
            serializationOptions.Converters.Add(new JSONDUTResultsConverter());

            await resultSemaphore.WaitAsync();

            //Update the API call with the new filter date range- Only check results since the last poll
            //use the dispatcher since we are not on the UI thread
            Dispatcher.Invoke(() => {
                UpdateAPICall();
                callURL = APICall3TextBlock.Text;
            });

            pollingStartDate = DateTime.Now; //Prepare for the next query- Only check results since the last poll

            //Check if the results have already been retrieved or the monitoring operation has been canceled
            if (!MonitoringForResult)
            {
                _ = resultSemaphore.Release();
                return;
            }

            try
            {
                //asynchronously call the server filter results API and parse the results
                //The list of FilteredDUTInformation objects contains all of the parsed data according to the classes defined above.
                filteredDUTsInformation = await serverConnection.GetFromJsonAsync<List<FilteredDUTInformation>>(callURL, serializationOptions);

                //Check if the filtered results contain a DUT with the target serial number
                targetDUT = filteredDUTsInformation.FirstOrDefault(dutInformation => CheckSerialNumberMatchesTargetSerialNumber(dutInformation.DUTSerialNumber));

                if (targetDUT != null) //we found the serial number
                {
                    //use the dispatcher since we are not on the UI thread
                    Dispatcher.Invoke(() => {
                        _ = MessageBox.Show("Polling backup received result. Retrieving full set of results for DUT from the server.");

                        //Use the dispatcher since we change UI element properties within the monitoringForResultProperty
                        MonitoringForResult = false;

                        //Form the call URL with the correct serial number- this is for the case where the target serial number is not case sensitive. All serial numbers in the server are case sensitive.
                        callURL = "http://" + parent.ServerAddressTextBox.Text + ":8083/integration/results?dutSn=" + targetDUT.DUTSerialNumber;
                    });

                    try
                    {
                        //asynchronously call the server get results API and parse the results
                        //The list of DUTResults objects contains all of the parsed data according to the classes defined above.
                        DUTResults = await serverConnection.GetFromJsonAsync<List<DUTResults>>(callURL, serializationOptions);

                        // Serialize the DUT results to JSON text to format and display
                        // This is done solely for displaying the JSON text in the sample program
                        rawJSON = JsonSerializer.Serialize(DUTResults, serializationOptions);

                        //format, colorize, and display the JSON
                        //use the dispatcher since we are not on the UI thread
                        Dispatcher.Invoke(() => {
                            JSONResponseRichTextBox.Document.Blocks.Add(jsonProcessor.FormatAndColorize(rawJSON));
                        });

                        StopMonitoringForResult();
                    }
                    // With the sample code, "An error occurred while sending the request." usually means the server address is incorrect.
                    // "Response status code does not indicate success: 400 (Bad request)" usually means there are no measurements corresponding to the input serial number
                    // In general, a bad request indicates there is a syntax or parameter error with API call.
                    catch (HttpRequestException ex)
                    {
                        _ = MessageBox.Show(ex.Message);
                    }
                }
            }
            // With the sample code, "An error occurred while sending the request." usually means the server address is incorrect.
            // "Response status code does not indicate success: 400 (Bad request)" usually means there are no measurements corresponding to the input serial number
            // In general, a bad request indicates there is a syntax or parameter error with API call.
            catch (HttpRequestException ex)
            {
                _ = MessageBox.Show(ex.Message);
            }

            _ = resultSemaphore.Release();
        }
        private async void CancelButton_Click(object sender, RoutedEventArgs e)
        {
            await resultSemaphore.WaitAsync();

            if (MonitoringForResult)
            {
                MonitoringForResult = false;
                StopMonitoringForResult();
            }

            _ = MessageBox.Show("Canceled result monitoring.");

            _ = resultSemaphore.Release();
        }

        private void StopMonitoringForResult()
        {
            //Stop polling for results
            if (pollingTimer != null)
            {
                pollingTimer.Dispose();
                pollingTimer = null;
            }

            //Stop listening for new results from the server
            if (resultsListener.IsListening)
            {
                resultsListener.Stop();
                resultsListener = null;
            }
        }

        private void DUTSerialNumberTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            UpdateAPICall();
        }
    }
}

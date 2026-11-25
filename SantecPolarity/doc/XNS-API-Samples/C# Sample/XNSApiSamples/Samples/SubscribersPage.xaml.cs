using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Net.Http.Json;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
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
    /// Represents subscriber information for subscribing to an XNS server
    /// </summary>
    public class Subscriber
    {
        /// <summary>
        /// The URL to send any new results received by the XNS server.
        /// </summary>
        [JsonPropertyName("url")]
        public string URL { get; set; }
    }

    /// <summary>
    /// Represents the full set of results (RLM and PTM) for a DUT when a new result is pushed to a XNS results subscriber.
    /// </summary>
    class NewResults
    {
        [JsonPropertyName("data")]
        public DUTCombinedMeasurements DUTCombinedMeasurements { get; set; }
    }

    /// <summary>
    /// Interaction logic for SubscribersPage.xaml
    /// </summary>
    public partial class SubscribersPage : Page, IAPICallSample
    {
        MainWindow parent;
        JSONProcessor jsonProcessor;
        HttpListener resultsListener;

        string localIPAddress;

        public SubscribersPage(MainWindow parent)
        {
            IPAddress[] iPAddresses;

            InitializeComponent();

            this.parent = parent;

            jsonProcessor = new JSONProcessor();

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
        }

        public string SampleName
        {
            get
            {
                return "Subscribers";
            }
        }

        /// <summary>
        /// Update the API call based on the server address
        /// </summary>
        /// <remarks> This method exists solely to support the sample program UI. </remarks>
        public void UpdateAPICall()
        {
            //ex. http://localhost:8083/api/subscribers/create-subscriber
            APICallTextBlock.Text = "http://" + parent.ServerAddressTextBox.Text + ":8083/api/subscribers/create-subscriber";
        }

        private async void SetSubscriber_Click(object sender, RoutedEventArgs e)
        {
            HttpClient serverConnection;
            JsonSerializerOptions serializationOptions;

            SetSubscriberButton.IsEnabled = false;

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

            Subscriber payload = new Subscriber { URL = "http://" + localIPAddress + ":8555" };

            try
            {
                //asynchronously call the XNS Server to POST the new subscriber
                _ = await serverConnection.PostAsJsonAsync<Subscriber>(APICallTextBlock.Text, payload);

                _ = MessageBox.Show("Subscribed successfully. The latest results will be shown below.");

                //run our local httpserver to receive notifications from the XNS Server
                resultsListener = new HttpListener();

                resultsListener.Prefixes.Add("http://*:" + "8555/");

                resultsListener.Start();

                _ = resultsListener.BeginGetContext(NewDUTCombinedMeasurementsCallback, null);
            }
            // With the sample code, "An error occurred while sending the request." usually means the server address is incorrect.
            // In general, a bad request indicates there is a syntax or parameter error with API call.
            catch(HttpRequestException ex)
            {
                _ = MessageBox.Show(ex.Message);

                SetSubscriberButton.IsEnabled = true;
            }

            SetSubscriberButton.IsEnabled = true;
        }

        private void NewDUTCombinedMeasurementsCallback(IAsyncResult result)
        {
            HttpListenerContext context;
            JsonSerializerOptions serializationOptions;
            NewResults newResults;
            string content;
            string rawJSON;

            //Setup the JSON serializer options
            //PropertyNameCaseInsensitive is set so that properties in the parsed objects do not need to match the JSON property names exactly for letter casing
            serializationOptions = new JsonSerializerOptions()
            {
                PropertyNameCaseInsensitive = true
            };

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

            //clear the response text- this is for displaying the JSON within the sample program
            //use the dispatcher since we are not on the UI thread
            Dispatcher.Invoke(() => {
                JSONResponseRichTextBox.Document.Blocks.Clear();
            });

            try
            {
                //The DUTCombinedMeasurements object contains all of the parsed data according to the classes defined in GetMeasurementsPage.xaml.cs.
                newResults = JsonSerializer.Deserialize<NewResults>(content, serializationOptions);

                // Serialize the DUT measurements to JSON text to format and display
                // This is done solely for displaying the JSON text in the sample program
                rawJSON = JsonSerializer.Serialize(newResults.DUTCombinedMeasurements, serializationOptions);

                //format, colorize, and display the JSON
                Dispatcher.Invoke(() => {
                    JSONResponseRichTextBox.Document.Blocks.Add(jsonProcessor.FormatAndColorize(rawJSON));
                });

                //indicate a success in the response to the server
                context.Response.StatusCode = (int)HttpStatusCode.OK;
                context.Response.Close();
            }
            catch (Exception)
            {
                //indicate an error in the response to the server
                context.Response.StatusCode = (int)HttpStatusCode.InternalServerError;
                context.Response.Close();
            }

            _ = resultsListener.BeginGetContext(NewDUTCombinedMeasurementsCallback, null);
        }
    }
}

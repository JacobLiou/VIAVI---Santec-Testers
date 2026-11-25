using System;
using System.Collections.Generic;
using System.Linq;
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
    ///
    /// Note: The filter results API returns results for a maximum of 10000 DUTs.
    /// </remarks>

    /// <summary>
    /// Represents information about a DUT and it's corresponding test results that have been filtered by the Filter Results API.
    /// </summary>
    class FilteredDUTInformation
    {
        /// <summary>
        /// The serial number of the DUT.
        /// </summary>
        [JsonPropertyName("serialNumber")]
        public string DUTSerialNumber { get; set; }

        /// <summary>
        /// The timestamp of when the DUT was created (results first received for the DUT by the XNS server).
        /// </summary>
        [JsonPropertyName("createdAt")]
        public DateTime DUTCreationTimestamp { get; set; }

        /// <summary>
        /// Information about the sets of test results that have been filtered by the Filter Results API for the DUT.
        /// </summary>
        [JsonPropertyName("testBlockResults")]
        public List<FilteredResultsInformation> FilteredResultsInformation  {get; set; }
    }

    /// <summary>
    /// Represents information about a set of test results that has been filtered by the Filter Results API.
    /// </summary>
    class FilteredResultsInformation
    {
        [JsonPropertyName("id")]
        public int ResultID { get; set; }

        [JsonPropertyName("testBlockId")]
        public int TestBlockID { get; set; }

        [JsonPropertyName("deviceId")]
        public int DeviceID { get; set; }

        [JsonPropertyName("type")]
        [JsonConverter(typeof(JSONDUTResultsTypeConverter))]
        public EDUTResultsType ResultsType { get; set; }

        /// <summary>
        /// The overall pass/fail result for the set of test block results.
        /// </summary>
        [JsonPropertyName("summary")]
        public bool OverallPassed { get; set; }

        /// <summary>
        /// The overall test status of the DUT. This is more detailed than just pass/fail as it includes incomplete status as well.
        /// </summary>
        [JsonPropertyName("testStatus")]
        [JsonConverter(typeof(JSONTestStatusConverter))]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public ETestStatus? TestStatus { get; set; }

        /// <summary>
        /// The custom field values for the test.
        /// The keys of the dictionary represent the custom field names and the values of the dictionary represent the custom field values.
        /// </summary>
        [JsonPropertyName("fieldResponses")]
        //[JsonConverter(typeof(JSONCustomFieldValuesConverter))] fieldresponse was converted from strings to proper key/value pairs, no need for custom converter as of XNS v1.38+
        public Dictionary<string, string> CustomFieldValues { get; set; }

        /// <summary>
        /// The station name of the station the test was run on.
        /// </summary>
        [JsonPropertyName("stationName")]
        public string StationName { get; set; }

        /// <summary>
        /// The timestamp of when the set of results was created (received by the XNS server).
        /// </summary>
        [JsonPropertyName("createdAt")]
        public DateTime ResultsCreationTimestamp { get; set; }
    }

    /// <summary>
    /// Interaction logic for FilterResultsPage.xaml
    /// </summary>
    public partial class FilterResultsPage : Page, IAPICallSample
    {
        MainWindow parent;
        JSONProcessor jsonProcessor;

        public FilterResultsPage(MainWindow parent)
        {
            InitializeComponent();

            this.parent = parent;

            jsonProcessor = new JSONProcessor();

            StartDateTextBox.Text = DateTime.Today.ToString("o");
            EndDateTextBox.Text = (DateTime.Today + TimeSpan.FromDays(1)).ToString("o");
        }

        public string SampleName
        {
            get
            {
                return "Filter Results";
            }
        }

        /// <summary>
        /// Update the API call based on the server address and the filter date range.
        /// </summary>
        /// <remarks> This method exists solely to support the sample program UI. </remarks>
        public void UpdateAPICall()
        {

            //ex. http://localhost:8083/integeration/results/filter-results?filterParams={"testDate":{"startDate":"2022-03-24 00:00:00","endDate":"2022-03-25 23:59:59"}}
            APICallTextBlock.Text = "http://" + parent.ServerAddressTextBox.Text + ":8083/integration/results/filter-results?filterParams={\"testDate\":{\"startDate\":\"";

            if (DateTime.TryParse(StartDateTextBox.Text, out DateTime startDate))
            {
                APICallTextBlock.Text += startDate.ToString("o");
            }

            APICallTextBlock.Text += "\",\"endDate\":\"";

            if (DateTime.TryParse(EndDateTextBox.Text, out DateTime endDate))
            {
                APICallTextBlock.Text += endDate.ToString("o");
            }

            APICallTextBlock.Text += "\"}}";
        }

        private async void FilterResultsButton_Click(object sender, RoutedEventArgs e)
        {
            HttpClient serverConnection;
            JsonSerializerOptions serializationOptions;
            List<FilteredDUTInformation> filteredDUTsInformation;
            string rawJSON;

            FilterResultsButton.IsEnabled = false;

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

            try
            {
                //asynchronously call the server filter results API and parse the results
                //The list of FilteredDUTInformation objects contains all of the parsed data according to the classes defined above.
                //Note: The filter results API returns results for a maximum of 10000 DUTs.
                filteredDUTsInformation = await serverConnection.GetFromJsonAsync<List<FilteredDUTInformation>>(APICallTextBlock.Text, serializationOptions);

                // Serialize the filtered DUT information to JSON text to format and display
                // This is done solely for displaying the JSON text in the sample program
                rawJSON = JsonSerializer.Serialize(filteredDUTsInformation, serializationOptions);

                //format, colorize, and display the JSON
                JSONResponseRichTextBox.Document.Blocks.Add(jsonProcessor.FormatAndColorize(rawJSON));
            }
            // With the sample code, "An error occurred while sending the request." usually means the server address is incorrect.
            // "Response status code does not indicate success: 400 (Bad request)" usually means there are no measurements corresponding to the input serial number
            // In general, a bad request indicates there is a syntax or parameter error with API call.
            catch (HttpRequestException ex)
            {
                _ = MessageBox.Show(ex.Message);
            }

            FilterResultsButton.IsEnabled = true;
        }

        private void EndDateTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            UpdateAPICall();
        }

        private void StartDateTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            UpdateAPICall();
        }
    }
}

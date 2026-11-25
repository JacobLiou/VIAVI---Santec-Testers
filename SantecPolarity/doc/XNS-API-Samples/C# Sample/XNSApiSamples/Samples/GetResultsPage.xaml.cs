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
    /// </remarks>

    /// <summary>
    /// Represents the type (IL/RL or Polarity) of a set of DUT results returned by the XNS server
    /// </summary>
    enum EDUTResultsType
    {
        ILRL,
        Polarity
    }

    // JSON converters
    /// <summary>
    /// Provides a converter for converting DUT results type string values in a JSON object to a corresponding DUT results type enumeration value
    /// </summary>
    class JSONDUTResultsTypeConverter : JsonConverter<EDUTResultsType>
    {
        /// <summary>
        /// Provides the conversion from string to EDUTResultsType value when reading JSON
        /// </summary>
        public override EDUTResultsType Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            string measurementType;

            // The measurement types are string values in the JSON
            measurementType = reader.GetString();

            switch (measurementType)
            {
                // Older results have RL1 as the results type while newer results have IL/RL as the results type
                case "RL1":
                case "IL/RL":
                    return EDUTResultsType.ILRL;
                // Older results have PT1 as the results type while newer results have Polarity as the results type
                case "PT1":
                case "Polarity":
                    return EDUTResultsType.Polarity;
            }

            throw new NotImplementedException();
        }

        //The write method is implemented to support displaying the JSON in the sample program
        public override void Write(Utf8JsonWriter writer, EDUTResultsType value, JsonSerializerOptions options)
        {
            switch (value)
            {
                case EDUTResultsType.ILRL:
                    writer.WriteStringValue("IL/RL");
                    break;
                case EDUTResultsType.Polarity:
                    writer.WriteStringValue("Polarity");
                    break;
                default:
                    throw new NotImplementedException();

            }
        }
    }

    /// <summary>
    /// Provides a converter for converting an RLM result set wavelength from a string value in the JSON to an integer value.
    /// </summary>
    class JSONWavelengthConverter : JsonConverter<int>
    {
        /// <summary>
        /// Provides the conversion from string to integer value when reading JSON
        /// </summary>
        public override int Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            return int.Parse(reader.GetString());
        }

        //The write method is implemented to support displaying the JSON in the sample program
        public override void Write(Utf8JsonWriter writer, int value, JsonSerializerOptions options)
        {
            writer.WriteStringValue(value.ToString());
        }
    }

    /// <summary>
    /// Provides a converter for deserializing the DUT results.
    ///
    /// This converter is required to support polymorphic deserialization of the DUT results into a list of both RLM results and PTM results.
    /// </summary>
    class JSONDUTResultsConverter : JsonConverter<List<DUTResults>>
    {
        /// <summary>
        /// Provides the conversion from JSON to a list of DUT result sets when deserializing DUT results.
        /// </summary>
        public override List<DUTResults> Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            List<DUTResults> dutResults = new List<DUTResults>();
            Utf8JsonReader clonedReader = reader;

            _ = clonedReader.Read(); //start array token

            while (reader.TokenType != JsonTokenType.EndArray)
            {
                if (reader.TokenType == JsonTokenType.PropertyName)
                {
                    //check the "type" of the result set to determine if it is a set of RLM results or a set of PTM results.
                    if (reader.GetString() == "type")
                    {
                        _ = reader.Read();

                        switch (reader.GetString())
                        {
                            // Older results have RL1 as the results type while newer results have IL/RL as the results type
                            case "RL1":
                            case "IL/RL":
                                dutResults.Add(JsonSerializer.Deserialize<RLMDUTResults>(ref clonedReader));
                                break;
                            // Older results have PT1 as the results type while newer results have Polarity as the results type
                            case "PT1":
                            case "Polarity":
                                dutResults.Add(JsonSerializer.Deserialize<PTMDUTResults>(ref clonedReader));
                                break;
                        }

                        _ = clonedReader.Read();
                    }
                    else
                    {
                        _ = reader.Read();

                        //read any arrays so we don't end processing the list prematurely
                        if (reader.TokenType == JsonTokenType.StartArray)
                        {
                            ReadArray(ref reader);
                        }
                    }
                }

                _ = reader.Read();
            }

            return dutResults;
        }

        private void ReadArray(ref Utf8JsonReader reader)
        {
            while (reader.TokenType != JsonTokenType.EndArray)
            {
                _ = reader.Read();

                if (reader.TokenType == JsonTokenType.StartArray)
                {
                    ReadArray(ref reader);

                    _ = reader.Read();
                }
            }
        }

        //The write method is implemented to support displaying the JSON in the sample program
        public override void Write(Utf8JsonWriter writer, List<DUTResults> value, JsonSerializerOptions options)
        {

            writer.WriteStartArray();

            foreach(DUTResults dutResults in value)
            {
                if (dutResults is RLMDUTResults rlmDUTResults)
                {
                    writer.WriteRawValue(JsonSerializer.Serialize<RLMDUTResults>(rlmDUTResults));
                }
                else if (dutResults is PTMDUTResults ptmDUTResults)
                {
                    writer.WriteRawValue(JsonSerializer.Serialize<PTMDUTResults>(ptmDUTResults));
                }
            }

            writer.WriteEndArray();
        }
    }

    // DUT Results
    /// <summary>
    /// Information about the DUT for a set of test block results.
    /// </summary>
    class DUTInformation
    {
        [JsonPropertyName("serialNumber")]
        public string SerialNumber { get; set; }

        /// <summary>
        /// The timestamp of when the DUT was created (first results for DUT received by the XNS server).
        /// </summary>
        [JsonPropertyName("createdAt")]
        public DateTime DUTCreationTimestamp { get; set; }
    }

    /// <summary>
    /// Information about the test plan for a set of test block results.
    /// </summary>
    class TestPlanInformation
    {
        [JsonPropertyName("name")]
        public string Name { get; set; }

        [JsonPropertyName("uuid")]
        public string UUID { get; set; }
    }

    /// <summary>
    /// Information about the test block for a set of test block results.
    /// </summary>
    class TestBlockInformation
    {
        [JsonPropertyName("name")]
        public string Name { get; set; }

        [JsonPropertyName("uuid")]
        public string UUID { get; set; }
    }

    /// <summary>
    /// Represents a set of results (RLM or PTM) from a single test block for a DUT.
    /// </summary>
    abstract class DUTResults
    {
        [JsonPropertyName("id")]
        public int ResultID { get; set; }

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
        /// Information about the DUT that the test was run on.
        /// </summary>
        [JsonPropertyName("testDut")]
        [JsonPropertyOrder(2)]
        public DUTInformation DUTInformation { get; set; }

        /// <summary>
        /// Information about the test run on the DUT.
        /// </summary>
        [JsonPropertyName("testPlan")]
        [JsonPropertyOrder(2)]
        public TestBlockInformation TestPlanInformation { get; set; }

        /// <summary>
        /// Information about the test run on the DUT.
        /// </summary>
        [JsonPropertyName("testBlock")]
        [JsonPropertyOrder(2)]
        public TestBlockInformation TestBlockInformation { get; set; }

        /// <summary>
        /// The timestamp of when the results were created (received by the XNS server).
        /// </summary>
        [JsonPropertyName("createdAt")]
        [JsonPropertyOrder(2)]
        public DateTime ResultsCreationTimestamp { get; set; }
    }

    /// <summary>
    /// Represents a set of RLM results from a single test block for a DUT.
    /// </summary>
    class RLMDUTResults : DUTResults
    {
        /// <summary>
        /// A list of the fiber measurements for the DUT
        /// </summary>
        [JsonPropertyName("fibers")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        [JsonPropertyOrder(1)]
        public List<FiberMeasurements> FiberMeasurements { get; set; }

        /// <summary>
        /// The results for all pass fail specifications
        /// The dictionary keys are the specifications (ex. {1a:1310:il}<0.15) with the value representing whether the DUT passed the specification
        /// </summary>
        [JsonPropertyName("testResults")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        [JsonPropertyOrder(1)]
        public Dictionary<string, bool> PassFailResults { get; set; }
    }

    /// <summary>
    /// Represents the set of RLM measurements for a particular fiber in a set of RLM results
    ///
    /// This class is required because the Get Measurements API and Get Results API return results/measurements in the same format but with different naming.
    /// </summary>
    class RLMResultsFiberMeasurements
    {
        //Serializer options can be used to support non-case sensitive matches
        //Case sensitive properties are only specified in the sample to support displaying the raw JSON
        [JsonPropertyName("fiber")]
        public string Fiber { get; set; }

        [JsonPropertyName("measurements")]
        public IReadOnlyList<RLMResultsFiberMeasurementsForWavelength> MeasurementsByWavelength { get; set; }
    }

    /// <summary>
    /// Represents the set of RLM measurements for a particular wavelength for a fiber
    ///
    /// This class is required because the Get Measurements API and Get Results API return the Wavelength field in different formats.
    /// The Get Results API returns the wavelength as a string value.
    /// The Get Results and Get Measurements APIs could share an implementation with a single converter that can handle both string and number values but separate classes are used in order to display the JSON as it is received in each case (as either a string or a number).
    /// </summary>
    class RLMResultsFiberMeasurementsForWavelength
    {
        [JsonPropertyName("wavelength")]
        [JsonConverter(typeof(JSONWavelengthConverter))]
        public int Wavelength { get; set; }

        [JsonPropertyName("results")]
        public IReadOnlyList<Measurement> Measurements { get; set; }
    }

    /// <summary>
    /// Represents a set of PTM results from a single test block for a DUT.
    /// </summary>
    class PTMDUTResults : DUTResults
    {
        /// <summary>
        /// The expected output to detector mapping.
        /// </summary>
        [JsonPropertyName("expectedMap")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        [JsonPropertyOrder(1)]
        public List<int> ExpectedMap { get; set; }

        /// <summary>
        /// The actual detected output to detector mapping.
        /// Entries of 0 represent outputs that failed to map to any detector.
        /// </summary>
        [JsonPropertyName("detectedMap")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        [JsonPropertyOrder(1)]
        public List<int> DetectedMap { get; set; }

        /// <summary>
        /// The corresponding polarity fibers for the entries in the detected map. Incomplete polarity tests will have fewer entries than the expected map.
        /// </summary>
        [JsonPropertyName("polarityFibers")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        [JsonPropertyOrder(1)]
        public List<int> PolarityFibers { get; set; }
    }

    /// <summary>
    /// Interaction logic for GetResultsPage.xaml
    /// </summary>
    public partial class GetResultsPage : Page, IAPICallSample
    {
        MainWindow parent;
        JSONProcessor jsonProcessor;

        public GetResultsPage(MainWindow parent)
        {
            InitializeComponent();

            this.parent = parent;

            jsonProcessor = new JSONProcessor();
        }

        public string SampleName
        {
            get
            {
                return "Get Results";
            }
        }

        /// <summary>
        /// Update the API call based on the server address and DUT serial number.
        /// </summary>
        /// <remarks> This method exists solely to support the sample program UI. </remarks>
        public void UpdateAPICall()
        {
            //ex. http://localhost:8083/integeration/results?dutSn=12345678
            APICallTextBlock.Text = "http://" + parent.ServerAddressTextBox.Text + ":8083/integration/results?dutSn=" + DUTSerialNumberTextBox.Text;
        }

        private async void GetResultsButton_Click(object sender, RoutedEventArgs e)
        {
            HttpClient serverConnection;
            JsonSerializerOptions serializationOptions;
            List<DUTResults> DUTResults ;
            string rawJSON;

            GetResultsButton.IsEnabled = false;

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

            //Add the converter for handling deserializing the list of DUT result sets.
            serializationOptions.Converters.Add(new JSONDUTResultsConverter());

            //clear the response text- this is for displaying the JSON within the sample program
            JSONResponseRichTextBox.Document.Blocks.Clear();

            try
            {
                //asynchronously call the server get results API and parse the results
                //The list of DUTResults objects contains all of the parsed data according to the classes defined above.
                DUTResults = await serverConnection.GetFromJsonAsync<List<DUTResults>>(APICallTextBlock.Text, serializationOptions);

                // Serialize the DUT results to JSON text to format and display
                // This is done solely for displaying the JSON text in the sample program
                rawJSON = JsonSerializer.Serialize(DUTResults, serializationOptions);

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

            GetResultsButton.IsEnabled = true;
        }

        private void DUTSerialNumberTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            UpdateAPICall();
        }
    }
}

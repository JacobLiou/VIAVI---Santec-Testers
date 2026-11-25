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
    /// Represents RLM measurement types returned by the XNS server
    /// </summary>
    enum EMeasurementType
    {
        RL,
        IL,
        RLtotal,
        ILtotal,
        Length
    }

    /// <summary>
    /// Represents test statuses returned by the XNS server
    /// </summary>
    enum ETestStatus
    {
        Pass,
        Fail,
        Incomplete
    }

    // General JSON converters
    /// <summary>
    /// Provides a converter for converting test information custom field values from a string value to a dictionary of the values
    /// </summary>
    class JSONCustomFieldValuesConverter : JsonConverter<Dictionary<string, string>>
    {
        /// <summary>
        /// Provides the conversion from string to boolean value when reading the JSON
        /// </summary>
        public override Dictionary<string, string> Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            _ = reader.GetString();
            return JsonSerializer.Deserialize<Dictionary<string, string>>(reader.GetString());
        }

        // The write method is implemented to support displaying the JSON in the sample program
        public override void Write(Utf8JsonWriter writer, Dictionary<string, string> value, JsonSerializerOptions options)
        {
            writer.WriteStringValue(JsonSerializer.Serialize(value));
        }
    }

    /// <summary>
    /// Provides a converter for converting test status string values in a JSON object to a corresponding test status enumeration value
    /// </summary>
    class JSONTestStatusConverter : JsonConverter<ETestStatus>
    {
        /// <summary>
        /// Provides the conversion from string to ETestStatus value when reading JSON
        /// </summary>
        public override ETestStatus Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            string testStatus;

            // The test statuses are string values in the JSON
            testStatus = reader.GetString();

            switch(testStatus)
            {
                case "TEST_STATUS__PASS":
                    return ETestStatus.Pass;
                case "TEST_STATUS__FAIL":
                    return ETestStatus.Fail;
                case "TEST_STATUS__INCOMPLETE":
                    return ETestStatus.Incomplete;
            }

            throw new NotImplementedException();
        }

        // The write method is implemented to support displaying the JSON in the sample program
        public override void Write(Utf8JsonWriter writer, ETestStatus value, JsonSerializerOptions options)
        {
            switch(value)
            {
                case ETestStatus.Pass:
                    writer.WriteStringValue("TEST_STATUS__PASS");
                    break;
                case ETestStatus.Fail:
                    writer.WriteStringValue("TEST_STATUS__FAIL");
                    break;
                case ETestStatus.Incomplete:
                    writer.WriteStringValue("TEST_STATUS__INCOMPLETE");
                    break;
                default:
                    throw new NotImplementedException();

            }
        }
    }

    // RLM JSON converters
    /// <summary>
    /// Provides a converter for converting measurement type string values in a JSON object to a corresponding measurement type enumeration value
    /// </summary>
    class JSONMeasurementTypeConverter : JsonConverter<EMeasurementType>
    {
        /// <summary>
        /// Provides the conversion from string to EMeasurementType value when reading JSON
        /// </summary>
        public override EMeasurementType Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            string measurementType;

            // The measurement types are string values in the JSON
            measurementType = reader.GetString();

            switch(measurementType)
            {
                case "rl":
                    return EMeasurementType.RL;
                case "il":
                    return EMeasurementType.IL;
                case "rltotal":
                    return EMeasurementType.RLtotal;
                case "iltotal":
                    return EMeasurementType.ILtotal;
                case "length":
                    return EMeasurementType.Length;
            }

            throw new NotImplementedException();
        }

        // The write method is implemented to support displaying the JSON in the sample program
        public override void Write(Utf8JsonWriter writer, EMeasurementType value, JsonSerializerOptions options)
        {
            switch(value)
            {
                case EMeasurementType.RL:
                    writer.WriteStringValue("rl");
                    break;
                case EMeasurementType.IL:
                    writer.WriteStringValue("il");
                    break;
                case EMeasurementType.RLtotal:
                    writer.WriteStringValue("rltotal");
                    break;
                case EMeasurementType.ILtotal:
                    writer.WriteStringValue("iltotal");
                    break;
                case EMeasurementType.Length:
                    writer.WriteStringValue("length");
                    break;
                default:
                    throw new NotImplementedException();

            }
        }
    }

    // PTM JSON Converters
    /// <summary>
    /// Provides a converter for converting the PTM overall pass/fail result from a string value to a boolean value
    /// </summary>
    class JSONPTMOverallPassedConverter : JsonConverter<bool?>
    {
        bool useNewFormat = false;

        /// <summary>
        /// Provides the conversion from string to boolean value when reading the JSON
        /// </summary>
        public override bool? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            string overallPassed;

            // On older versions of XNS, the PTM overall passed value is a string
            // On newer versions of XNS, the PTM overall passed value is a boolean
            if(reader.TokenType == JsonTokenType.String)
            {
                useNewFormat = false;

                overallPassed = reader.GetString();

                if(overallPassed == "Pass")
                {
                    return true;
                }
                else if(overallPassed == "Fail")
                {
                    return false;
                }
            }
            else
            {
                useNewFormat = true;
                return reader.GetBoolean();
            }

            return null;
        }

        // The write method is implemented to support displaying the JSON in the sample program

        public override void Write(Utf8JsonWriter writer, bool? value, JsonSerializerOptions options)
        {
            if(useNewFormat && value != null)
            {
                writer.WriteBooleanValue(value.Value);
            }
            else
            {
                if(value == true)
                {
                    writer.WriteStringValue("Pass");
                }
                else if(value == false)
                {
                    writer.WriteStringValue("Fail");
                }
            }
        }
    }

    // DUT Combined Measurements
    /// <summary>
    /// Represents the full set of most recent combined results (RLM and PTM) for a DUT
    /// </summary>
    class DUTCombinedMeasurements
    {
        [JsonPropertyName("dutSerialNumber")]
        public string DUTSerialNumber { get; set; }

        [JsonPropertyName("rl1")]
        public RLMResults RLMResults { get; set; }

        [JsonPropertyName("pt1")]
        public PTMResults PTMResults { get; set; }

        [JsonPropertyName("legacy")]
        public List<object> LegacyResults { get; set; }
    }

    // General Results
    /// <summary>
    /// Additional information about where a test was run and any custom field values
    /// </summary>
    class TestInformation
    {
        /// <summary>
        /// The station name of the station the test was run on.
        /// </summary>
        [JsonPropertyName("station")]
        public string StationName { get; set; }

        /// <summary>
        /// The name of the test plan that was run.
        /// </summary>
        [JsonPropertyName("testPlan")]
        public string TestPlanName { get; set; }

        /// <summary>
        /// The name of the test block that was run.
        /// </summary>
        [JsonPropertyName("testBlock")]
        public string TestBlockName { get; set; }

        /// <summary>
        /// The custom field values for the test.
        /// The keys of the dictionary represent the custom field names and the values of the dictionary represent the custom field values.
        /// </summary>
        [JsonPropertyName("fieldResponses")]
        //[JsonConverter(typeof(JSONCustomFieldValuesConverter))] fieldresponse was converted from strings to proper key/value pairs, no need for custom converter as of XNS v1.38+
        public Dictionary<string, string> CustomFieldValues { get; set; }

        /// <summary>
        /// The date and time the test was run at.
        /// </summary>
        [JsonPropertyName("measurementDate")]
        public string MeasureDate { get; set; }

        /// <summary>
        /// The UUID of the test plan that was run.
        /// </summary>
        [JsonPropertyName("testPlanUUID")]
        public string TestPlanUUID { get; set; }

        /// <summary>
        /// The UUID of the test block that was run.
        /// </summary>
        [JsonPropertyName("testBlockUUID")]
        public string TestBlockUUID { get; set; }

        /// <summary>
        /// The station name of the station the test was run on.
        /// </summary>
        [JsonPropertyName("deviceSerialNumber")]
        public string DeviceSN { get; set; }
    }

    // RLM Results
    /// <summary>
    /// Results and info specific to the RLM Return Loss Meter.
    ///
    /// The RLM results can be returned as an empty JSON object. In this case, all properties within the RLMResults object will be set to null.
    ///
    /// The JSON property names and ignore conditions are solely to support displaying the JSON within the sample program.
    /// </summary>
    class RLMResults
    {
        /// <summary>
        /// A list of the fiber measurements for the DUT
        /// </summary>
        [JsonPropertyName("fibers")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public List<FiberMeasurements> FiberMeasurements { get; set; }

        /// <summary>
        /// The results for all pass fail specifications
        /// The dictionary keys are the specifications (ex. {1a:1310:il}<0.15) with the value representing whether the DUT passed the specification
        /// </summary>
        [JsonPropertyName("testResults")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public Dictionary<string, bool> PassFailResults { get; set; }

        /// <summary>
        /// The overall pass/fail result of the DUT
        /// </summary>
        [JsonPropertyName("status")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public bool? OverallPassed { get; set; }

        /// <summary>
        /// The overall test status of the DUT. This is more detailed than just pass/fail as it includes incomplete status as well.
        /// </summary>
        [JsonPropertyName("testStatus")]
        [JsonConverter(typeof(JSONTestStatusConverter))]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public ETestStatus? TestStatus { get; set; }

        /// <summary>
        /// Additional test information about the tests performed on the DUT, including the station, test block run, and any custom field values
        /// </summary>
        [JsonPropertyName("info")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public List<TestInformation> TestInformation { get; set; }

        /// <summary>
        /// Whether all tests have been completed for the DUT
        /// For test plans with multiple RLM test blocks, this will return true only if there are results for all RLM test blocks.
        /// </summary>
        [JsonPropertyName("isTestCompleted")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public bool? IsTestCompleted { get; set; }
    }

    /// <summary>
    /// Represents the set of RLM measurements for a particular fiber
    /// </summary>
    class FiberMeasurements
    {
        //Serializer options can be used to support non-case sensitive matches
        //Case sensitive properties are only specified in the sample to support displaying the raw JSON
        [JsonPropertyName("fiber")]
        public string Fiber { get; set; }

        [JsonPropertyName("measurements")]
        public IReadOnlyList<FiberMeasurementsForWavelength> MeasurementsByWavelength { get; set; }
    }

    /// <summary>
    /// Represents the set of RLM measurements for a particular wavelength for a fiber
    /// </summary>
    class FiberMeasurementsForWavelength
    {
        [JsonPropertyName("wavelength")]
        public int Wavelength { get; set; }

        [JsonPropertyName("results")]
        public IReadOnlyList<Measurement> Measurements { get; set; }
    }

    /// <summary>
    /// Represents an RLM measurement.
    /// </summary>
    class Measurement
    {
        // We use a converter to convert the measurement type from a string to an enumeration value
        [JsonPropertyName("type")]
        [JsonConverter(typeof(JSONMeasurementTypeConverter))]
        public EMeasurementType Type { get; set; }

        [JsonPropertyName("value")]
        public double Value { get; set; }
    }

    // PTM Results
    /// <summary>
    /// Results and info specific to the PTM Polarity Tester.
    ///
    /// The PTM results can be returned as an empty JSON object. In this case, all properties within the PTMResults object will be set to null.
    ///
    /// The JSON property names and ignore conditions are solely to support displaying the JSON within the sample program.
    /// </summary>
    class PTMResults
    {
        /// <summary>
        /// The expected output to detector mapping.
        /// </summary>
        [JsonPropertyName("expectedMap")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public List<int> ExpectedMap { get; set; }

        /// <summary>
        /// The actual detected output to detector mapping.
        /// Entries of 0 represent outputs that failed to map to any detector.
        /// </summary>
        [JsonPropertyName("detectedMap")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public List<int> DetectedMap { get; set; }

        /// <summary>
        /// The corresponding polarity fibers for the entries in the detected map. Incomplete polarity tests will have fewer entries than the expected map.
        /// </summary>
        [JsonPropertyName("polarityFibers")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public List<int> PolarityFibers { get; set; }

        /// <summary>
        /// The overall pass/fail result of the polarity test.
        /// This is only null when there are no PTM results and an empty JSON object is returned for PTMResults.
        /// </summary>
        [JsonPropertyName("status")]
        [JsonConverter(typeof(JSONPTMOverallPassedConverter))]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public bool? OverallPassed { get; set; }

        /// <summary>
        /// The overall test status of the DUT. This is more detailed than just pass/fail as it includes incomplete status as well.
        /// </summary>
        [JsonPropertyName("testStatus")]
        [JsonConverter(typeof(JSONTestStatusConverter))]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public ETestStatus? TestStatus { get; set; }

        /// <summary>
        /// Additional test information about the tests performed on the DUT, including the station, test block run, and any custom field values
        /// </summary>
        [JsonPropertyName("info")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public List<TestInformation> TestInformation { get; set; }
    }

    /// <summary>
    /// Interaction logic for GetMeasurementsPage.xaml
    /// </summary>
    public partial class GetMeasurementsPage : Page, IAPICallSample
    {
        MainWindow parent;
        JSONProcessor jsonProcessor;

        public GetMeasurementsPage(MainWindow parent)
        {
            InitializeComponent();

            this.parent = parent;

            jsonProcessor = new JSONProcessor();
        }

        public string SampleName
        {
            get
            {
                return "Get Measurements";
            }
        }

        /// <summary>
        /// Update the API call based on the server address and DUT serial number.
        /// </summary>
        /// <remarks> This method exists solely to support the sample program UI. </remarks>
        public void UpdateAPICall()
        {
            //ex. http://localhost:8083/integeration/results/combined-measurements?dutSn=12345678
            APICallTextBlock.Text = "http://" + parent.ServerAddressTextBox.Text + ":8083/integration/results/combined-measurements?dutSn=" + DUTSerialNumberTextBox.Text;
        }

        private async void GetMeasurementsButton_Click(object sender, RoutedEventArgs e)
        {
            HttpClient serverConnection;
            JsonSerializerOptions serializationOptions;
            DUTCombinedMeasurements DUTCombinedMeasurements;
            string rawJSON;

            GetMeasurementsButton.IsEnabled = false;

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
                //asynchronously call the server get measurements API and parse the results
                //the DUTCombinedMeasurements object contains all of the parsed data according to the classes defined above.
                DUTCombinedMeasurements = await serverConnection.GetFromJsonAsync<DUTCombinedMeasurements>(APICallTextBlock.Text, serializationOptions);

                // Serialize the DUT measurements to JSON text to format and display
                // This is done solely for displaying the JSON text in the sample program
                rawJSON = JsonSerializer.Serialize(DUTCombinedMeasurements, serializationOptions);

                //format, colorize, and display the JSON
                JSONResponseRichTextBox.Document.Blocks.Add(jsonProcessor.FormatAndColorize(rawJSON));
            }
            // With the sample code, "An error occurred while sending the request." usually means the server address is incorrect.
            // "Response status code does not indicate success: 400 (Bad request)" usually means there are no measurements corresponding to the input serial number
            // In general, a bad request indicates there is a syntax or parameter error with API call.
            catch(HttpRequestException ex)
            {
                _ = MessageBox.Show(ex.Message);
            }

            GetMeasurementsButton.IsEnabled = true;
        }

        private void DUTSerialNumberTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            UpdateAPICall();
        }
    }
}

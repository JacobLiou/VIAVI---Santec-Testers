This project contains sample code for using the XNS server API.

The XNS server must be installed on a computer. You will need the IP address of the computer the XNS server is installed on.
You can use "localhost" if the XNS server is installed on the same machine the sample program is running on.

For any example API calls, the sample code for executing the call can be found entirely within the corresponding page code file.
ex. For the "Get Measurements" API call, the code can be found in Samples\GetMeasurementsPage.xaml.cs

The sample code uses the System.Net.Http.Json package (https://www.nuget.org/packages/System.Net.Http.Json) to deserialize JSON directly to C# objects.
The received JSON response from the XNS server is displayed in the sample program UI after an API call has been executed.

MainWindow.xaml.cs, Samples\IAPICallSample.cs, and JSONProcessor contain code for supporting the sample program UI and can otherwise be ignored.
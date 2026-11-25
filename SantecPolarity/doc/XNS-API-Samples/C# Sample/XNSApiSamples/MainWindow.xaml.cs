using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
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
using XNSApiSamples.Samples;

namespace XNSApiSamples
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    /// <remarks>
    /// This class only contains code to support the sample program UI logic.
    /// The appropriate sample code can be found in each sample code page in the samples folder.
    /// ex. GetMeasurementsPage.xaml.cs
    /// Note: The sample code uses the System.Net.Http.Json package (https://www.nuget.org/packages/System.Net.Http.Json).
    /// </remarks>
    public partial class MainWindow : Window
    {
        // store all the samples
        List<IAPICallSample> samples;

        public MainWindow()
        {
            samples = new List<IAPICallSample>();

            InitializeComponent();

            samples.Add(new GetMeasurementsPage(this));
            samples.Add(new GetResultsPage(this));
            samples.Add(new FilterResultsPage(this));
            samples.Add(new SubscribersPage(this));
            samples.Add(new MonitorForResultPage(this));

            samplesListView.ItemsSource = samples;

            samplesListView.SelectedItem = samples.First();

            foreach (IAPICallSample sample in samples)
            {
                sample.UpdateAPICall();
            }
        }

        private void ServerAddressTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            //update all the sample API calls since the server address has changed
            foreach(IAPICallSample sample in samples)
            {
                sample.UpdateAPICall();
            }
        }

        private void ListView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            sampleFrame.Content = samplesListView.SelectedItem;
        }
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XNSApiSamples.Samples
{
    /// <summary>
    /// Represents an API call sample.
    /// </summary>
    /// <remarks> This class exists solely to support the sample program UI and can be ignored. </remarks>
    interface IAPICallSample
    {
        /// <summary>
        /// This represents the name to display in the samples list view.
        /// </summary>
        string SampleName { get; }

        /// <summary>
        /// Update the sample API call.
        /// </summary>
        void UpdateAPICall();
    }
}

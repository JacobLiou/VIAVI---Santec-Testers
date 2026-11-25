using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows.Documents;
using System.Windows.Media;

namespace XNSApiSamples
{
    /// <summary>
    /// Provides a rotating set of brushes for colorizing text.
    /// </summary>'
    /// <remarks>
    /// This class exists solely to support the sample program UI.
    /// </remarks>
    class BrushProvider
    {
        List<Color> colors;

        public BrushProvider()
        {
            colors = new List<Color>
            {
                Colors.Orange,
                Colors.DeepPink,
                Colors.LawnGreen,
                Colors.Purple
            };
        }

        public Brush GetBrush(int indentLevel)
        {
            return new SolidColorBrush(colors[indentLevel % colors.Count]);
        }
    }

    /// <summary>
    /// Provides a processor to format and colorize raw JSON text.
    /// </summary>'
    /// <remarks>
    /// This class exists solely to support the sample program UI.
    /// </remarks>
    class JSONProcessor
    {
        public Paragraph FormatAndColorize(string rawJSON)
        {
            int indentLevel = 0;
            bool isNamed = false;

            Paragraph paragraph;
            Run run;

            Utf8JsonReader reader;

            BrushProvider brushProvider = new BrushProvider();

            reader = new Utf8JsonReader(Encoding.UTF8.GetBytes(rawJSON));

            paragraph = new Paragraph();

            try
            {
                while (reader.Read())
                {
                    switch (reader.TokenType)
                    {
                        case JsonTokenType.StartArray:
                            run = new Run
                            {
                                Text = "[" + Environment.NewLine,
                                Foreground = brushProvider.GetBrush(indentLevel)
                            };

                            indentLevel += 1;
                            isNamed = false;
                            paragraph.Inlines.Add(run);
                            break;
                        case JsonTokenType.EndArray:
                            indentLevel -= 1;

                            run = new Run
                            {
                                Text = GetIndentString(indentLevel) + "]" + Environment.NewLine,
                                Foreground = brushProvider.GetBrush(indentLevel)
                            };

                            isNamed = false;
                            paragraph.Inlines.Add(run);
                            break;
                        case JsonTokenType.StartObject:
                            run = new Run();

                            if (!isNamed)
                            {
                                run.Text = GetIndentString(indentLevel);
                            }

                            run.Text += run.Text = "{" + Environment.NewLine;

                            run.Foreground = brushProvider.GetBrush(indentLevel);
                            indentLevel += 1;
                            isNamed = false;
                            paragraph.Inlines.Add(run);
                            break;
                        case JsonTokenType.EndObject:
                            run = new Run();
                            indentLevel -= 1;
                            run.Text = GetIndentString(indentLevel) + "}" + Environment.NewLine;
                            run.Foreground = brushProvider.GetBrush(indentLevel);
                            isNamed = false;
                            paragraph.Inlines.Add(run);
                            break;
                        case JsonTokenType.PropertyName:
                            run = new Run
                            {
                                Text = GetIndentString(indentLevel) + reader.GetString(),
                                Foreground = new SolidColorBrush(Colors.Goldenrod)
                            };
                            paragraph.Inlines.Add(run);

                            run = new Run
                            {
                                Text = " : ",
                                Foreground = new SolidColorBrush(Colors.White)
                            };
                            paragraph.Inlines.Add(run);
                            isNamed = true;
                            break;
                        case JsonTokenType.String:
                            run = new Run();

                            if (!isNamed)
                            {
                                run.Text = GetIndentString(indentLevel);
                            }

                            run.Text += "\"" + reader.GetString() + "\"" + Environment.NewLine;
                            run.Foreground = new SolidColorBrush(Colors.DeepSkyBlue);
                            paragraph.Inlines.Add(run);
                            isNamed = false;
                            break;
                        case JsonTokenType.Number:
                            run = new Run();

                            if (!isNamed)
                            {
                                run.Text = GetIndentString(indentLevel);
                            }

                            run.Text += reader.GetDouble() + Environment.NewLine;
                            run.Foreground = new SolidColorBrush(Colors.MediumOrchid);
                            paragraph.Inlines.Add(run);
                            isNamed = false;
                            break;
                        case JsonTokenType.True:
                            run = new Run();

                            if (!isNamed)
                            {
                                run.Text = GetIndentString(indentLevel);
                            }

                            run.Text += reader.GetBoolean().ToString() + Environment.NewLine;
                            run.Foreground = new SolidColorBrush(Colors.LightGreen);
                            paragraph.Inlines.Add(run);
                            isNamed = false;
                            break;
                        case JsonTokenType.False:
                            run = new Run();

                            if (!isNamed)
                            {
                                run.Text = GetIndentString(indentLevel);
                            }

                            run.Text += reader.GetBoolean().ToString() + Environment.NewLine;
                            run.Foreground = new SolidColorBrush(Colors.Crimson);
                            paragraph.Inlines.Add(run);
                            isNamed = false;
                            break;
                        case JsonTokenType.Null:
                            run = new Run();

                            if (!isNamed)
                            {
                                run.Text = GetIndentString(indentLevel);
                            }

                            run.Text += "null" + Environment.NewLine;
                            run.Foreground = new SolidColorBrush(Colors.BlanchedAlmond);
                            paragraph.Inlines.Add(run);
                            isNamed = false;
                            break;
                    }
                }
            }
            catch (Exception)
            {
                //swallow exception
            }

            return paragraph;
        }

        private string GetIndentString(int indentLevel)
        {
            string indent = string.Empty;

            for (int i = 0; i < indentLevel; i++)
            {
                indent += "    ";
            }

            return indent;
        }
    }
}

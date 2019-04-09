using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using Xceed.Wpf.Toolkit;
using System.Windows.Controls.Primitives;

namespace RastrChartWPFControl
{
    internal class ColorPickerLocalized : ColorPicker
    {
        private ToggleButton _advanced;
        public override void OnApplyTemplate()
        {
            base.OnApplyTemplate();
            _advanced = GetTemplateChild("_colorMode") as ToggleButton;
            if (_advanced != null)
            {
                _advanced.Content = Resources["AdvancedPicker"].ToString();
                _advanced.Unchecked += (sender, e) =>
                {
                    _advanced.Content = Resources["AdvancedPicker"].ToString();

                };
                _advanced.Checked += (sender, e) =>
                {
                    _advanced.Content = Resources["StandardPicker"].ToString();

                };
            }
        }
    } 

    internal class LinePanel : StackPanel
    {
        private Line PreviewLine;
        private ComboBox comboBox;
        
        internal LinePanel(ComboBox Combo, int DashStyle)
        {
            comboBox = Combo;
            comboBox.SizeChanged += OnSizeChanged;
            comboBox.LayoutUpdated += OnSizeChanged;
            SizeChanged += OnSizeChanged;
            PreviewLine = new Line();
            PreviewLine.Stroke = Brushes.Black;
            PreviewLine.StrokeThickness = 2;
            PreviewLine.StrokeDashArray = LineDashStyles.GetDashStyle(DashStyle);

            PreviewLine.X1 = 0;
            PreviewLine.X2 = comboBox.ActualWidth;
            Children.Add(PreviewLine);
        }
        protected void OnSizeChanged(object sender, EventArgs e)
        {
            PreviewLine.X1 = 0;
            PreviewLine.X2 = comboBox.ActualWidth;
            PreviewLine.Y1 = PreviewLine.Y2 = Height * 0.6;
        }
    }

    internal partial class ChannelPropsWindow : Window
    {
        private ChartChannel channel;
        private AxisYBlock axisblock;

        internal ChannelPropsWindow(ChartChannel Channel, AxisYBlock Axisblock)
        {
            channel = Channel;
            axisblock = Axisblock;
            InitializeComponent();
        }

        internal ChannelPropsWindow()
        {
            InitializeComponent();
        }

        private void OnLoad(object sender, RoutedEventArgs e)
        {
            ChannelLegend.Text = channel.LegendText;
            string strChannelThickness = channel.LineThickness.ToString();
            foreach (ComboBoxItem cbi in Thickness.Items)
            {
                if (cbi.Content.ToString() == strChannelThickness)
                {
                    Thickness.SelectedItem = cbi;
                    break;
                }
            }

            Label measureLabel = new Label();
            measureLabel.Margin = measureLabel.Padding = new Thickness(0);
            measureLabel.Content = "8";
            measureLabel.Measure(new Size(Double.PositiveInfinity, Double.PositiveInfinity));

            for (int dx = 0; dx < LineDashStyles.StylesCount(); dx++)
            {
                ComboBoxItem cbi = new ComboBoxItem();
                LinePanel cbipan = new LinePanel(LegendStyle,dx);
                cbipan.Width = Double.NaN;
                cbipan.Height = measureLabel.DesiredSize.Height;
                cbipan.Margin = new Thickness(0, 0, 0, 0);
                cbipan.MinWidth = 50;
                cbi.Content = cbipan;
                LegendStyle.Items.Add(cbi);
            }
            LegendColor.SelectedColor = channel.LineColor;
            LegendStyle.SelectedIndex = channel.LineDashStyle;

            ComboBoxItem MainAxis = new ComboBoxItem();
            MainAxis.Content = Resources["MainAxis"].ToString();

            ComboBoxItem NewAxis = new ComboBoxItem();
            NewAxis.Content = Resources["NewAuxAxis"].ToString();
            AxisIndex.Items.Add(MainAxis);

            string AuxTitle = Resources["AuxAxis"].ToString() + " ";

            int nCount = 1;
            foreach (AxisY axis in axisblock.Axes)
            {
                if (axis.MainAxis)
                    MainAxis.Tag = axis.Id;
                else
                {
                    ComboBoxItem AuxAxis = new ComboBoxItem();
                    AuxAxis.Content = AuxTitle + nCount.ToString();
                    nCount++;
                    AuxAxis.Tag = axis.Id;
                    AxisIndex.Items.Add(AuxAxis);
                }
                if (NewAxis.Tag == null)
                    NewAxis.Tag = axis.Id;
                else
                    if ((int)NewAxis.Tag < axis.Id)
                        NewAxis.Tag = axis.Id;
            }

            NewAxis.Tag = (int)NewAxis.Tag+1;
            
            AxisIndex.Items.Add(NewAxis);

            foreach (ComboBoxItem cbi in AxisIndex.Items)
            {
                if ((int)cbi.Tag == channel.Axis)
                {
                    AxisIndex.SelectedItem = cbi;
                    break;
                }
            }

            UnitsName.Text = channel.Units;

        }

        private void OnClickOK(object sender, RoutedEventArgs e)
        {
            channel.LegendText = ChannelLegend.Text;
            int thickness = 1;
            if(int.TryParse(Thickness.Text, out thickness))
                channel.LineThickness = thickness;
            channel.LineColor = LegendColor.SelectedColor.HasValue ? LegendColor.SelectedColor.Value : Colors.Black;
            channel.LineDashStyle = LegendStyle.SelectedIndex;
            channel.Axis = (int)((ComboBoxItem)AxisIndex.SelectedItem).Tag;
            channel.Units = UnitsName.Text;
            this.DialogResult = true;
        }

        private void OnClickCancel(object sender, RoutedEventArgs e)
        {
            this.DialogResult = false;
        }
    }
}

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
using System.Windows.Shapes;

namespace RastrChartWPFControl
{
    /// <summary>
    /// Interaction logic for AxisSetup.xaml
    /// </summary>
    public partial class AxisSetup : Window
    {
        private Axis axis_;
        private AxisConstraints axisConstraints_;
        private ChartCanvasPlotter chartPlotter_;

        string originalMin;
        string originalMax;
        string originalGridStep;

        public AxisSetup()
        {
            InitializeComponent();
        }

        internal AxisSetup(Axis axis, AxisConstraints axisConstraints, ChartCanvasPlotter chartPlotter)
        {
            InitializeComponent();
            axis_ = axis;
            axisConstraints_ = axisConstraints;
            chartPlotter_ = chartPlotter;
        }

        private void OnLoad(object sender, RoutedEventArgs e)
        {
            UpdateUI();
            originalMax = MaxBox.Text;
            originalMin = MinBox.Text;
            originalGridStep = GridBox.Text;
        }

        public void UpdateTextBox(TextBox textBox, double autoValue, object constraintValue)
        {
            if (constraintValue is double)
            {
                textBox.Text = constraintValue.ToString();
                textBox.Background = new SolidColorBrush(Colors.Orange);
            }
            else
            {
                textBox.Text = autoValue.ToString();
                textBox.Background = new SolidColorBrush(Colors.White);
            }
        }

        private void UpdateUI()
        {
            if (axis_ != null)
            {
                AxisData WorldSize = axis_.WorldMinMax();
                UpdateTextBox(MinBox, WorldSize.actualStart, axisConstraints_.ViewStart);
                UpdateTextBox(MaxBox, WorldSize.actualEnd, axisConstraints_.ViewEnd);
                UpdateTextBox(GridBox, WorldSize.step, axisConstraints_.GridStep);
            }
        }

        private void OnMinLostFocus(object sender, RoutedEventArgs e)
        {
            if (originalMin != MinBox.Text)
            {
                originalMin = MinBox.Text;
                if (double.TryParse(originalMin, out double result))
                {
                    axisConstraints_.ViewStart = result;
                    chartPlotter_.ZoomExtents();
                    UpdateUI();
                }
            }
        }

        private void OnMaxLostFocus(object sender, RoutedEventArgs e)
        {
            if (originalMax != MaxBox.Text)
            {
                originalMax = MaxBox.Text;
                if (double.TryParse(originalMax, out double result))
                {
                    axisConstraints_.ViewEnd = result;
                    chartPlotter_.ZoomExtents();
                    UpdateUI();
                }
            }
        }

        private void OnGridStepLostFocus(object sender, RoutedEventArgs e)
        {
            if (originalGridStep != GridBox.Text)
            {
                originalGridStep = GridBox.Text;
                if (double.TryParse(GridBox.Text, out double result))
                {
                    axisConstraints_.GridStep = result;
                    chartPlotter_.ZoomExtents();
                    UpdateUI();
                }
            }
        }

        private void OnClickOK(object sender, RoutedEventArgs e)
        {
            this.DialogResult = true;
        }

        private void OnMinAuto(object sender, RoutedEventArgs e)
        {
            axisConstraints_.ViewStart = null;
            chartPlotter_.ZoomExtents();
            UpdateUI();
        }

        private void OnMaxAuto(object sender, RoutedEventArgs e)
        {
            axisConstraints_.ViewEnd = null;
            chartPlotter_.ZoomExtents();
            UpdateUI();
        }

        private void OnGridStepAuto(object sender, RoutedEventArgs e)
        {
            axisConstraints_.GridStep = null;
            chartPlotter_.ZoomExtents();
            UpdateUI();
        }
    }
}

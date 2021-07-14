using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Shapes;
using System.Windows.Media;
using System.Windows.Controls;
using System.Windows.Input;


namespace RastrChartWPFControl
{
    internal class EventOverArgs : EventArgs
    {
        public bool Enter
        {
            get;
            set;
        }

        public EventOverArgs(bool bEnter)
        {
            Enter = bEnter;
        }
    }

    internal class LegendButton : Grid
    {
        private TextBlock legendText;
        private Line legendLine;
        private Color legendColor;
        

        public event EventHandler ButtonOver;
        public event EventHandler ButtonClick;
        public event EventHandler ButtonRightClick;

        public LegendButton()
        {
            Margin = new Thickness(0,0,6,2);
            RowDefinition row0 = new RowDefinition();
            RowDefinition row1 = new RowDefinition();
            //row0.Height = new GridLength(70);
            //row1.Height = new GridLength(5);
            RowDefinitions.Add(row0);
            RowDefinitions.Add(row1);
            legendText = new TextBlock();
            legendText.Padding = new Thickness(5);
            legendLine = new Line();
            Grid.SetRow(legendText, 0);
            Grid.SetRow(legendLine, 1);
            LegendColor = Colors.Red;
            legendLine.StrokeThickness = 1;
            legendLine.X1 = legendLine.Y1 = 0;
            legendLine.X2 = 100;
            legendLine.Y2 = 0;
            Children.Add(legendLine);
            Children.Add(legendText);
            legendText.MouseEnter += OnLabelMouseEnter;
            legendText.MouseLeave += OnLabelMouseLeave;
            legendText.MouseLeftButtonDown += OnClick;
            legendText.MouseRightButtonDown += OnRightClick;
            Unloaded += LegendButton_Unloaded;

        }

        private void LegendButton_Unloaded(object sender, RoutedEventArgs e)
        {
            legendText.MouseEnter -= OnLabelMouseEnter;
            legendText.MouseLeave -= OnLabelMouseLeave;
            legendText.MouseLeftButtonDown -= OnClick;
            legendText.MouseRightButtonDown -= OnRightClick;
            Unloaded -= LegendButton_Unloaded;
        }

        private void OnButtonOver(object Sender, EventArgs e)
        {
            if (ButtonOver != null)
                ButtonOver(this, e);
        }

        private void OnButtonClick(object Sender, EventArgs e)
        {
            if (ButtonClick != null)
                ButtonClick(this, e);
        }

        private void OnButtonRightClick(object Sender, EventArgs e)
        {
            if (ButtonRightClick != null)
                ButtonRightClick(this, e);
        }

        public bool Highlight
        {
            set
            {
                if (value)
                {
                    LinearGradientBrush HighlightBrush = new LinearGradientBrush(legendColor, Colors.White, 90);
                    Background = HighlightBrush;
                }
                else
                    Background = Brushes.White;
            }
        }
        public string LegendText
        {
            get { return legendText.Text; }
            set 
            { 
                legendText.Text = value;
                legendText.Measure(new Size(Double.PositiveInfinity, Double.PositiveInfinity));
                legendLine.X2 = legendText.DesiredSize.Width;
            }
        }
        public Color LegendColor
        {
            get { return legendColor; }
            set { legendLine.Stroke = new SolidColorBrush(legendColor = value); }
        }

        public double LegendThickness
        {
            get { return legendLine.StrokeThickness; }
            set { legendLine.StrokeThickness = value; }
        }
        public int LegendStyle
        {
            set { legendLine.StrokeDashArray = LineDashStyles.GetDashStyle(value); }
        }

        protected void OnLabelMouseEnter(object sender, EventArgs e)
        {
            Cursor = Cursors.Hand;
            Highlight = true;
            OnButtonOver(this, new EventOverArgs(true));
        }

        protected void OnClick(object sender, EventArgs e)
        {
            OnButtonClick(sender, e);
        }

        protected void OnRightClick(object sender, EventArgs e)
        {
            OnButtonRightClick(sender, e);
        }

        protected void OnLabelMouseLeave(object sender, EventArgs e)
        {
            Cursor = Cursors.Arrow;
            Highlight = false;
            OnButtonOver(this, new EventOverArgs(false));
        }
    }
}

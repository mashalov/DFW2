using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Shapes;
using System.Windows.Media;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;

namespace RastrChartWPFControl
{
    internal class AxisData
    {
        public double actualStart;
        public double actualEnd;
        public double step;

        public AxisData(double ActualStart, double ActualEnd, double GridStep)
        {
            actualStart = ActualStart;
            actualEnd = ActualEnd;
            step = GridStep;
        }
    }

    public class AxisConstraints
    {
        private object viewStart;
        private object viewEnd;
        private object gridStep;

        public AxisConstraints()
        {
            Reset();
        }

        public object ViewStart
        {
            get { return viewStart; }
            set { viewStart = value; }
        }

        public object ViewEnd
        {
            get { return viewEnd; }
            set { viewEnd = value; }
        }

        public object GridStep
        {
            get { return gridStep; }
            set { gridStep = value; }
        }

        public void Reset()
        {
            viewStart = null;
            viewEnd = null;
            gridStep = null;
        }

        public bool IsSet
        {
            get { return viewStart is double || viewEnd is double || gridStep is double; }
        }


    }


    internal class InfoWindow : Popup
    {
        private TextBlock InfoText;
        private LinearGradientBrush back;
        private Border border;
        private Color currentColor;
        public InfoWindow()
        {
            
            border = new Border();
            border.BorderThickness = new Thickness(1);
            border.BorderBrush = Brushes.DarkGoldenrod;
            InfoText = new TextBlock();
            currentColor = Colors.LightYellow;
            back = new LinearGradientBrush(Colors.LightYellow, Colors.White, 90);
            back.Freeze();
            border.Child = InfoText;
            Child = border;
            InfoText.Background = back;
            InfoText.Padding = new Thickness(3);
        }
        public double Value
        {
            set
            {
                InfoText.Text = value.ToString("G7");
            }
        }

        public void SetValueAndColor(double value, Color newColor)
        {
            Value = value;
            if (newColor != currentColor)
            {
                border.BorderBrush = new SolidColorBrush(newColor);
                border.BorderThickness = new Thickness(3);
            }
        }

        public Size GetSize()
        {
            InfoText.Measure(new Size(Double.PositiveInfinity, Double.PositiveInfinity));
            return InfoText.DesiredSize;
        }
    }

    public class AxisInfo
    {
        public double Ratio;
        public double Offset;
    }

    internal class Axis : Canvas
    {
        protected Line AxisLine;
        protected Line HighlightAxisLine;
        protected TextBlock MeasureText;
        internal AxisData axisData;
        protected Point ZoomFactor;
        protected Point Offset;
        public Rect bounds;
        public bool bGotBounds;
        public Point Translation;
        internal InfoWindow infoWindow;
        public AxisConstraints AxisConstraints = null;


        public int Id
        {
            get;
            set;
        }

        public void Highlight(bool bHighLight, Color channelColor) 
        {
            if (bHighLight)
            {
                HighlightAxisLine.X1 = ActualWidth;
                HighlightAxisLine.X2 = ActualWidth;
                HighlightAxisLine.Y1 = 0;
                HighlightAxisLine.Y2 = ActualHeight;
                HighlightAxisLine.Stroke = new SolidColorBrush(channelColor);
                HighlightAxisLine.StrokeThickness = 7;
                HighlightAxisLine.Visibility = System.Windows.Visibility.Visible;
            }
            else
            {
                HighlightAxisLine.Visibility = System.Windows.Visibility.Hidden;
            }
        }

        public Axis()
        {
            ZoomFactor = new Point(1, 1);
            Offset = new Point(0, 0);
            AxisLine = new Line();
            axisData = new AxisData(0,0,0);
            AxisLine.Stroke = Brushes.Black;
            AxisLine.StrokeThickness = 1;
            AxisLine.SnapsToDevicePixels = true;
            Background = Brushes.White;
            MeasureText = new TextBlock();
            MeasureText.FontSize = 14;
            ClipToBounds = true;
            HighlightAxisLine = new Line();
            MinWidth = 50;
            bGotBounds = false;
            Translation = new Point(1, 0);
            infoWindow = new InfoWindow();
        }

        virtual protected double MeasureSize(double x)
        {
            return 0.0;
        }

        protected string ToString(double x)
        {
            // prevent long small number instead of 0.0;
            if (Math.Abs(x) < 1E-15) x = 0.0;

            return x.ToString("G7");
        }


        protected bool GenerateTickMarks(double Start, double End)
        {
            bool bRes = false;
            if (End - Start > 0)
            {
                double MarkSize = Math.Max(MeasureSize(Start),MeasureSize(End));

                while (true)
                {
                    double[] bases = { 5, 2, 1 };
                    double exponentMax = 0;
                    double mantissaMax = Math.Ceiling(End / Math.Pow(10.0, exponentMax));
                    double mantissaMin = Math.Floor(Start / Math.Pow(10.0, exponentMax));
                    int numberOfTicks = 1 + Math.Min(Convert.ToInt32(Math.Floor((End - Start) / MarkSize)), 10);
                    double realInt = (End - Start) / numberOfTicks;
                    int expInt = Convert.ToInt32(Math.Floor(Math.Log10(Math.Abs(realInt))));
                    double manInt = realInt / Math.Pow(10.0, expInt);

                    int ClosestIndex = 0;
                    for (int k = 1; k < 3; k++)
                    {
                        if (Math.Abs(manInt - bases[ClosestIndex]) > Math.Abs(manInt - bases[k]))
                            ClosestIndex = k;
                    }

                    axisData.step = bases[ClosestIndex] * Math.Pow(10, expInt);

                    if (AxisConstraints != null && AxisConstraints.GridStep is double)
                    {
                        if ((double)AxisConstraints.GridStep > 0)
                            axisData.step = (double)AxisConstraints.GridStep;
                        else
                            AxisConstraints.GridStep = axisData.step;
                    }
                                            

                    axisData.actualEnd = (1.0 + Math.Floor(End / axisData.step)) * axisData.step;
                    axisData.actualStart = axisData.actualEnd - (1.0 + Math.Floor((End - Start) / axisData.step)) * axisData.step;

                    double MaxWidth = 0;
                    for (double x = axisData.actualStart; x <= axisData.actualEnd; x += axisData.step)
                    {
                        double TestWidth = MeasureSize(x);
                        if (TestWidth > MaxWidth)
                            MaxWidth = TestWidth;
                    }
                    if (MaxWidth <= MarkSize)
                        break;
                    else
                        MarkSize = MaxWidth;
                }

                bRes = true;
            }
            return bRes;
        }

        public void SetTransform(Point Zoom, Point Offs)
        {
            ZoomFactor = Zoom;
            Offset = Offs;
            Update();
        }

        virtual protected void Update()
        {

        }

        virtual public void MeasurePoint(Point screenPoint)
        {
    
        }


        public event EventHandler TranslationChanged;

        public void OnTraslationChanged(object sender, EventArgs e)
        {
            if (TranslationChanged != null)
                TranslationChanged(sender, e);
        }

        public virtual AxisData WorldMinMax()
        {
            return new AxisData(0,0,axisData.step);
        }
    }

    internal class AxisX : Axis
    {
        public event EventHandler ButtonRightClick;

        public AxisX()
        {
            AxisConstraints = new AxisConstraints();
            MouseRightButtonDown += OnRightClick;
            MouseWheel += OnMouseWheel;
        }

        public override AxisData WorldMinMax()
        {
            return new AxisData(-Offset.X / ZoomFactor.X, (ActualWidth - Offset.X) / ZoomFactor.X, axisData.step);
        }
        override protected void Update()
        {
            if (ActualWidth > 0)
            {
                Children.Clear();
                Children.Add(HighlightAxisLine);
                Children.Add(AxisLine);
                AxisLine.X1 = 0;
                AxisLine.X2 = ActualWidth;
                AxisLine.Y1 = AxisLine.Y2 = 0;

                AxisData WorldSize = WorldMinMax();

                if (GenerateTickMarks(WorldSize.actualStart, WorldSize.actualEnd))
                {
                    for (double x = axisData.actualStart; x <= axisData.actualEnd; x += axisData.step)
                    {
                        TextBlock tx = new TextBlock();
                        tx.FontSize = MeasureText.FontSize;
                        tx.Text = ToString(x);
                        Children.Add(tx);
                        tx.Measure(new Size(Double.PositiveInfinity, Double.PositiveInfinity));
                        double X = x * ZoomFactor.X + Offset.X; 
                        SetLeft(tx, X - tx.DesiredSize.Width / 2.0);
                        SetTop(tx, 2);
                        Line ln = new Line();
                        ln.X1 = ln.X2 = X;
                        ln.Y1 = 0; ln.Y2 = 5;
                        ln.Stroke = AxisLine.Stroke;
                        ln.StrokeThickness = 1;
                        ln.SnapsToDevicePixels = true;
                        Children.Add(ln);
                    }
                }
            }
        }

        override protected double MeasureSize(double x)
        {
            MeasureText.Text = ToString(x);
            MeasureText.Text = new String('8',MeasureText.Text.Length+2);
            MeasureText.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
            return MeasureText.DesiredSize.Width / ZoomFactor.X * 1.5;
        }

        override public void MeasurePoint(Point screenPoint)
        {
            Point LeftTop = PointToScreen(new Point(0, 0));
            infoWindow.Placement = PlacementMode.Top;
            infoWindow.PlacementRectangle = new Rect(screenPoint.X + 2, LeftTop.Y - 2, 100, 150);
            Point ptt  = PresentationSource.FromVisual(this).CompositionTarget.TransformFromDevice.Transform(new Point(1,1));
            infoWindow.Value = (ptt.X * (screenPoint.X - LeftTop.X) - Offset.X) / ZoomFactor.X;
        }

        private void OnMouseWheel(object s, EventArgs e)
        {
            MouseWheelEventArgs me = (MouseWheelEventArgs)e;
            double ZoomChange = me.Delta > 0 ? 1.1 : 1 / 1.1;
            Translation.X = ZoomChange;
            OnTraslationChanged(this, e);
        }

        private void OnRightClick(object Sender, EventArgs e)
        {
            if (ButtonRightClick != null)
                ButtonRightClick(this, e);
        }
    }
    
    internal class AxisY: Axis
    {

        Point MouseDownPoint;
        bool bDrag;
        double OriginalYTranslate;
        protected TextBlock units;

        public AxisY()
        {
            MouseWheel += OnMouseWheel;
            MouseLeftButtonDown += OnMouseLBDown;
            MouseLeftButtonUp += OnMouseLBUp;
            MouseMove += OnMouseMove;
            LostMouseCapture += OnLostMouseCapture;
            MouseDownPoint = new Point();
            MouseEnter += OnMouseEnter;
            MouseLeave += OnMouseLeave;
            bDrag = false;
            MainAxis = false;
            units = new TextBlock();
            SolidColorBrush unitsBrush = new SolidColorBrush(Colors.White);
            unitsBrush.Opacity = 0.9;
            units.Background = unitsBrush;
            units.Padding = new Thickness(5,0,5,3);
            SetRight(units, 1);
            SetTop(units, 0);
            units.Text = "";
        }

        public bool MainAxis
        {
            get;
            set;
        }

        public double ScreenOffset
        {
            get;
            set;
        }

        public string Units
        {
            get { return units.Text; }
            set {

                if (value.Length > 0)
                {
                    string CurrentUnits = units.Text;
                    string[] UnitsArray = CurrentUnits.Split(new string[] { Environment.NewLine }, StringSplitOptions.None);
                    bool bAdd = true;
                    foreach (string str in UnitsArray)
                    {
                        if (str.ToUpper() == value.ToUpper())
                        {
                            bAdd = false;
                            break;
                        }

                    }
                    if (bAdd)
                    {
                        if (CurrentUnits.Length > 0)
                            CurrentUnits += Environment.NewLine;
                        CurrentUnits += value;
                    }
                    units.Text = CurrentUnits;
                }
                else
                {
                    units.Text = value;
                }
                Update();
            }
        }

        public override AxisData WorldMinMax()
        {
            return new AxisData((Offset.Y - ActualHeight) / ZoomFactor.Y, Offset.Y / ZoomFactor.Y, axisData.step);
        }

        override protected void Update()
        {
            if (ActualHeight > 0)
            {
                Children.Clear();
                
               
                AxisData WorldSize = WorldMinMax();

                if (GenerateTickMarks(-WorldSize.actualEnd, -WorldSize.actualStart))
                {
                    double testWidth = Double.NegativeInfinity;
                    for (double y = axisData.actualStart; y <= axisData.actualEnd; y += axisData.step)
                    {
                        MeasureText.Text = "-" + ToString(-y);
                        MeasureText.Measure(new Size(Double.PositiveInfinity,Double.PositiveInfinity));
                        if (testWidth < MeasureText.DesiredSize.Width)
                            testWidth = MeasureText.DesiredSize.Width;
                    }

                    double k = ActualWidth / (testWidth + 7);
                    if (k > 1.0) k = 1.0;

                    for (double y = axisData.actualStart; y <= axisData.actualEnd; y += axisData.step)
                    {
                        TextBlock tx = new TextBlock();
                        tx.FontSize = MeasureText.FontSize * k;
                        tx.Text = ToString(-y);
                        Children.Add(tx);
                        tx.Measure(new Size(Double.PositiveInfinity, Double.PositiveInfinity));
                        double Y = y * ZoomFactor.Y + Offset.Y;
                        SetTop(tx, Y - tx.DesiredSize.Height / 2.0);
                        SetRight(tx, 7);
                    }

                    if (units.Text.Length > 0)
                    {
                        units.Width = this.ActualWidth;
                        Children.Add(units);
                    }

                    Children.Add(HighlightAxisLine);
                    Children.Add(AxisLine);
                    AxisLine.X1 = AxisLine.X2 = ActualWidth-1;
                    AxisLine.Y1 = 0;
                    AxisLine.Y2 = ActualHeight;

                    for (double y = axisData.actualStart; y <= axisData.actualEnd; y += axisData.step)
                    {
                        double Y = y * ZoomFactor.Y + Offset.Y;
                        Line ln = new Line();
                        ln.X1 = ActualWidth;
                        ln.X2 = ActualWidth - 4;
                        ln.Y1 = Y; ln.Y2 = Y;
                        ln.Stroke = AxisLine.Stroke;
                        ln.StrokeThickness = 1;
                        ln.SnapsToDevicePixels = true;
                        Children.Add(ln);
                    }
                 }
              }
        }

        override protected double MeasureSize(double x)
        {
            MeasureText.Text = x.ToString();
            MeasureText.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
            return MeasureText.DesiredSize.Height * 1.5 / ZoomFactor.Y;
        }

        private void OnMouseWheel(object s, EventArgs e)
        {
            if (!MainAxis)
            {
                MouseWheelEventArgs me = (MouseWheelEventArgs)e;
                double ZoomChange = me.Delta > 0 ? 1.1 : 1 / 1.1;
                Point MouseCurrentPoint = me.GetPosition(this);
                double y0 = (MouseCurrentPoint.Y - Offset.Y) / (ZoomFactor.Y);
                Translation.Y += (1 - ZoomChange) * y0 * Translation.X;
                Translation.X *= ZoomChange;
                OnTraslationChanged(this, e);
            }
        }

        private void OnMouseLBDown(object s, EventArgs e)
        {
            if (!bDrag && !MainAxis)
            {
                MouseEventArgs me = (MouseEventArgs)e;
                MouseDownPoint = me.GetPosition(this);
                OriginalYTranslate = Translation.Y;
                CaptureMouse();
                bDrag = true;
            }
        }
        
        private void OnMouseLBUp(object s, EventArgs e)
        {
            if (!MainAxis)
            {
                bDrag = false;
                ReleaseMouseCapture();
            }
        }

        private void OnLostMouseCapture(object s, EventArgs e)
        {
            if (!MainAxis)
            {
                if (!IsMouseCaptured)
                    bDrag = false;
            }
        }


        private void OnMouseMove(object s, EventArgs e)
        {
            MouseEventArgs me = (MouseEventArgs)e;
            Point MouseCurrentPoint = me.GetPosition(this);

            if (bDrag && !MainAxis)
            {
                Translation.Y = OriginalYTranslate + (MouseCurrentPoint.Y - MouseDownPoint.Y) * Translation.X / ZoomFactor.Y;
                OnTraslationChanged(this, e);
            }
        }

        private void OnMouseEnter(object s, EventArgs e)
        {
            if(!MainAxis)
                Cursor = Cursors.Hand;
        }

        private void OnMouseLeave(object s, EventArgs e)
        {
            Cursor = Cursors.Arrow;
        }

        override public void MeasurePoint(Point screenPoint)
        {
            Point LeftTop = PointToScreen(new Point(0, 0));
            infoWindow.Placement = PlacementMode.Right;
            infoWindow.PlacementRectangle = new Rect(LeftTop.X+2, screenPoint.Y - ScreenOffset, 0, 50);
            Point ptt = PresentationSource.FromVisual(this).CompositionTarget.TransformFromDevice.Transform(new Point(1, 1));
            infoWindow.Value = -(ptt.X * (screenPoint.Y - LeftTop.Y) - Offset.Y) / ZoomFactor.Y;
        }
     }

    internal class AxisYBlock : StackPanel
    {
        private List<AxisY> axes;
        public event EventHandler ButtonRightClick;
        public AxisConstraints AxisConstraints = new AxisConstraints();

        public event EventHandler TranslationChanged;

        private void OnTraslationChanged(object sender, EventArgs e)
        {
            if (TranslationChanged != null)
                TranslationChanged(sender, e);
        }

        public AxisYBlock()
        {
            Orientation = System.Windows.Controls.Orientation.Horizontal;
            axes = new List<AxisY>();
            MouseRightButtonDown += OnRightClick;
        }

        internal AxisY GetMainAxis()
        {
            foreach(AxisY axis in axes)
                if (axis.MainAxis)
                    return axis;
            return null;
        }

        public bool CheckAxis(int nAxis)
        {
            foreach (AxisY axis in axes)
                if (axis.Id == nAxis)
                    return true;
            return false;
        }

        internal AxisY GetAxis(int nAxis)
        {
            AxisY foundAxis = null;
            foreach(AxisY axis in axes)
            {
                if (axis.Id == nAxis)
                {
                    foundAxis = axis;
                    break;
                }
            }
            if (foundAxis == null)
            {
                foundAxis = new AxisY();
                foundAxis.TranslationChanged += OnAxisTranslationChanged;
                foundAxis.Id = nAxis;
                if (axes.Count == 0)
                    foundAxis.MainAxis = true;
                axes.Add(foundAxis);
                Children.Insert(0,foundAxis);
            }
            return foundAxis;
        }

        public void RemoveAllAxes()
        {
            foreach (AxisY axis in axes)
            {
                axis.TranslationChanged -= OnAxisTranslationChanged;
                Children.Remove(axis);
            }
            axes.Clear();
        }

        public bool RemoveAxis(int nAxis)
        {
            bool bRes = false;
            foreach (AxisY axis in axes)
            {
                if (axis.Id == nAxis)
                {
                    axis.TranslationChanged -= OnAxisTranslationChanged;
                    Children.Remove(axis);
                    axes.Remove(axis);
                    if (axis.MainAxis)
                        if (axes.Count > 0)
                            axes[0].MainAxis = true;
                    bRes = true;
                    break;
                }
            }
            return bRes;
        }

        public void SetTransform(int nAxis, Point Zoom, Point Offs)
        {
            AxisY axis = GetAxis(nAxis);
            axis.SetTransform(Zoom, Offs);
        }

        public void Highlight(int nAxis, bool bHighlight, Color channelColor)
        {
            AxisY axis = GetAxis(nAxis);
            if(axes.Count > 1)
                axis.Highlight(bHighlight, channelColor);
        }

        internal AxisData GetAxisData(int nAxis)
        {
            AxisY axis = GetAxis(nAxis);
            return axis.axisData;
        }
        
        internal List<AxisY> Axes
        {
            get { return axes; }
        }

        public AxisInfo[] AxesInfo
        {
            get
            {
                AxisInfo[] axl = new AxisInfo[axes.Count];
                int Count = 0;
                foreach(AxisY axy in axes)
                {
                    axl[Count++] = new AxisInfo {
                                                    Ratio = axy.Translation.X,
                                                    Offset = axy.Translation.Y
                                                };
                }
                return axl;
            }

            set
            {
                
            }
        }


        protected void OnAxisTranslationChanged(object sender, EventArgs e)
        {
            OnTraslationChanged(sender, e);
        }

        public bool OrganizeAxes(List<int> ToAdd, List<int> ToRemove)
        {
            bool bRes = false;
            foreach(int id in ToRemove)
                bRes = bRes || RemoveAxis(id);
            if (axes.Count == 0)
                bRes = true;
            foreach(int id in ToAdd)
                GetAxis(id);
            return bRes;
        }

        public void MeasurePoint(Point screenPoint)
        {
            double ud = 1;
            double dOffs = 3;
            foreach(AxisY axis in Axes)
            {
                axis.MeasurePoint(screenPoint);
                axis.ScreenOffset = ud * axis.infoWindow.GetSize().Height - dOffs + ud * (dOffs + 1) *2;
                axis.infoWindow.IsOpen = true;
                ud = ud == 1 ? 0 : 1;
            }
        }

        private void OnRightClick(object Sender, EventArgs e)
        {
            if (axes.Count == 1)
            {
                if (ButtonRightClick != null)
                    ButtonRightClick(this, e);
            }
        }
    }
}

	
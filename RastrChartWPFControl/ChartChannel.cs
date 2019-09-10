using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Media.Effects;

namespace RastrChartWPFControl
{

    internal static class LineDashStyles
    {
        private static List<DoubleCollection> dashStyles;
        static LineDashStyles()
        {
            dashStyles = new List<DoubleCollection>();
            dashStyles.Add(null);
            dashStyles.Add(new DoubleCollection(new double[] { 1, 1}));
            dashStyles.Add(new DoubleCollection(new double[] { 1, 2 }));
            dashStyles.Add(new DoubleCollection(new double[] { 5, 3 }));
        }

        internal static DoubleCollection GetDashStyle(int index)
        {
            if (IndexAvailable(index))
                return dashStyles[index];
            else
                return null;
        }

        internal static bool IndexAvailable(int index)
        {
            return (index >= 0 && index < dashStyles.Count) ? true : false;
        }

        public static int StylesCount()
        {
            return dashStyles.Count;
        }
    }
    
    internal class IntPair
    {
        public int x0;
        public int x1;
        public IntPair(int X0, int X1)
        {
            x0 = X0; x1 = X1;
        }
    }

    public class ChartChannel
    {
        internal Point[] OriginalPoints;
        private Point[] TransformedPoints;
        private Points outerPoints;
        private StreamGeometry LineStream;
        internal Path LinePath;
        private Color ChannelColor;
        private Rect internalBounds;
        private Point OldZoomFactor;
        internal LegendButton legendButton;
        private double lineThickness;
        private ChartCanvasPlotter plotter;
        private int nAxis;
        private int dashStyle;
        private string units = "";
        private string legend = "";
        private string tag = "";
        private bool bDirty = true;


        internal event EventHandler ChannelOver;
        internal event EventHandler ChannelClick;
        internal event EventHandler AxisChanged;
        internal event EventHandler LegendMenu;
        internal event EventHandler PointsChanged;

        private InfoWindow infoWindow;
        private Line linePointer;

        private List<FrameworkElement> elements;
       
        public string Tag
        {
            get 
            { 
                return tag; 
            }
            set
            {
                if (tag != value)
                {
                    if (plotter.GetChannelByTag(value) == null)
                        tag = value;
                    else
                        throw new Exception(String.Format("Channel tag \"{0}\" is already in use", value));
                }
            }
        }


        public bool Pin
        {
            get;
            set;
        }

        public string Units
        {
            get { return units;}
            set {
                    if (units != value)
                    {
                        units = value;
                        SetLegendButtonText();
                        OnAxisChanged(this, new EventArgs());
                    }
                }
        }

        public bool ToFlash
        {
            get; 
            set;
        }

        /// <summary>
        /// Gets or sets axis id for the channel. Channels with same Axis values will be placed on common separate axis
        ///</summary>
        public int Axis
        {
            get { return nAxis; }
            set
                {
                    if (value != nAxis)
                    {
                        nAxis = value;
                        OnAxisChanged(this, new EventArgs());
                    }
                }
        }

        public double LineThickness
        {
            get { 
                    return lineThickness; 
                }
            set { 
                    LinePath.StrokeThickness = lineThickness = value;
                    legendButton.LegendThickness = value;
                }
        }

        public Color LineColor
        {
            get { 
                    return ChannelColor; 
                }
            set { 
                    ChannelColor = value;
                    SolidColorBrush brush = new SolidColorBrush(ChannelColor);
                    LinePath.Stroke = brush;
                    legendButton.LegendColor = ChannelColor;
                }
        }

        public bool CanBeReference
        {
            get ; set ;
        }

        internal string LegendButtonText
        {
            get { return legendButton.LegendText; }
        }

        private void SetLegendButtonText()
        {
            legendButton.LegendText = legend;
            if (units.Length > 0)
                legendButton.LegendText += ",[" + units + "]";
        }

        public string LegendText
        {
            get {
                    return legend;
                }
            set
                {
                    legend = value;
                    SetLegendButtonText();
                }
        }

        public int LineDashStyle
        {
            get {
                    return dashStyle;
                }
            set
                {
                    if (LineDashStyles.IndexAvailable(value))
                    {
                        dashStyle = value;
                        LinePath.StrokeDashArray = LineDashStyles.GetDashStyle(dashStyle);
                        legendButton.LegendStyle = dashStyle;
                    }
                }
        }

        internal bool Highlight
        {
            set
            {
                if (value)
                {
                    DropShadowEffect de = new DropShadowEffect();
                    de.Color = LineColor;
                    de.ShadowDepth = 2;
                    de.Opacity = 1;
                    de.Direction = 0;
                    de.BlurRadius = 8;
                    lineThickness = LinePath.StrokeThickness;
                    LinePath.StrokeThickness = lineThickness + 2;
                    LinePath.Effect = de;
                    //Panel.SetZIndex(LinePath, 10000);
                }
                else
                {
                    LinePath.Effect = null;
                    LineThickness = lineThickness;
                }
            }
        }

        internal Rect Bounds
        {
            get
            {
                Point lt = new Point(double.PositiveInfinity,double.PositiveInfinity);
                Point rb = new Point(double.NegativeInfinity, double.NegativeInfinity);
                if (OriginalPoints != null)
                {
                    foreach(Point pt in OriginalPoints)
                    {
                        if (pt.X < lt.X)
                            lt.X = pt.X;
                        if (pt.X > rb.X)
                            rb.X = pt.X;

                        if (-pt.Y < lt.Y)
                            lt.Y = -pt.Y;
                        if (-pt.Y > rb.Y)
                            rb.Y = -pt.Y;
                    }
                }
                internalBounds.X = Math.Min(lt.X,rb.X);
                internalBounds.Y = Math.Min(lt.Y,rb.Y);
                double w = internalBounds.Width = Math.Max(lt.X,rb.X) - internalBounds.X;
                double h = internalBounds.Height = (Math.Max(lt.Y,rb.Y) - internalBounds.Y);
               
                if (h > 0)
                {
                    internalBounds.Y -= h * 0.05;
                    internalBounds.Height += h * 0.1;
                }
                /*
                else
                {
                    internalBounds.Y -= 0.5;
                    internalBounds.Height = 1;
                }
                */
                
                if (Math.Abs(w) < 1E-5)
                {
                    internalBounds.X -= 0.5;
                    internalBounds.Width = 1;
                }
                return internalBounds;
            }
        }
 
        internal ChartChannel(ChartCanvasPlotter Plotter)
        {
            plotter = Plotter;
            LineStream = new StreamGeometry();
            LinePath = new Path();
            LinePath.StrokeLineJoin = PenLineJoin.Bevel;
            legendButton = new LegendButton();
            LineColor = Colors.Black;
            LineThickness = 1;
            internalBounds = new Rect();
            infoWindow = new InfoWindow();
            outerPoints = new Points();
            linePointer = new Line();
            linePointer.StrokeThickness = 1;
            linePointer.Visibility = Visibility.Hidden;

            outerPoints.PointsChanged += OnPointsChanged;

            LineDashStyle = 0;
            
            LinePath.MouseEnter += OnMouseEnter;
            LinePath.MouseLeave += OnMouseLeave;
            legendButton.ButtonOver += OnButtonOver;
            legendButton.ButtonClick += OnButtonClick;
            legendButton.ButtonRightClick += OnButtonRightClick;

            //FilterRedunantPoints(.0001);
            
            elements = new List<FrameworkElement>();
            elements.Add(infoWindow);
            elements.Add(linePointer);

            CanBeReference = false;
            Pin = false;
        }

        private IntPair GetRange(double Xmin, double Xmax)
        {
            IntPair ret = new IntPair(0, -1);
            if (TransformedPoints != null && TransformedPoints.Length > 0)
            {
                if (TransformedPoints[0].X >= Xmin)
                    ret.x0 = 0;
                else
                {
                    int beg = 0;
                    int end = TransformedPoints.Length - 1;

                    while (end-beg > 1)
                    {
                        int mid = beg + (end - beg) / 2;
                        if (TransformedPoints[mid].X > Xmin)
                            end = mid;
                        else
                            beg = mid;
                    }
                    ret.x0 = beg;
                }

                if (TransformedPoints[TransformedPoints.Length - 1].X < Xmax)
                    ret.x1 = TransformedPoints.Length - 1;
                else
                {
                    int beg = 0;
                    int end = TransformedPoints.Length - 1;

                    while (end - beg > 1)
                    {
                        int mid = beg + (end - beg) / 2;
                        if (TransformedPoints[mid].X < Xmax)
                            beg = mid;
                        else
                            end = mid;
                    }
                    ret.x1 = end;
                }
            }
            //System.Diagnostics.Trace.WriteLine(string.Format("[{0}][{1}]", ret.x0, ret.x1));
            return ret;
        }

        internal void SetTransform(Point Scale, Point Offset, double ScreenWidth, double ScreenHeight)
        {
            double WorldXMax = (ScreenWidth - Offset.X) / Scale.X;
            double WorldXMin = (-Offset.X) / Scale.X;

            TransformPoints(Scale);

            IntPair range = GetRange(WorldXMin, WorldXMax);
            if (range.x1 > range.x0)
            {
                LineStream.Clear();
                using (StreamGeometryContext ctx = LineStream.Open())
                {
                    ctx.BeginFigure(TransformedPoints[range.x0],false, false);
                    for (int i = range.x0 + 1; i <= range.x1 ; i++)
                        ctx.LineTo(TransformedPoints[i], true, false);
                }

                LineStream.FillRule = FillRule.EvenOdd;
                TransformGroup tg = new TransformGroup();
                ScaleTransform st = new ScaleTransform(Scale.X, Scale.Y);
                TranslateTransform tt = new TranslateTransform(Offset.X, Offset.Y);
                tg.Children.Add(st);
                tg.Children.Add(tt);
                LineStream.Transform = tg;
                LinePath.Data = LineStream;
            }
        }

        private void TransformPoints(Point Scale)
        {
            if(OriginalPoints != null && OriginalPoints.Length > 0)
            {
                if (Scale != OldZoomFactor || bDirty)
                {
                    OldZoomFactor = Scale;
                    bDirty = false;
                    Point[] tempPoint = new Point[OriginalPoints.Length];
                    int PrevAdded = 0;
                    int nCount = 1;
                    tempPoint[PrevAdded].X = OriginalPoints[0].X;
                    tempPoint[PrevAdded].Y = -OriginalPoints[0].Y;
                    for (int i = 1; i < OriginalPoints.Length; i++)
                    {
                        double dx = OriginalPoints[PrevAdded].X;
                        double dy = OriginalPoints[PrevAdded].Y;
                        dx -= OriginalPoints[i].X;
                        dy -= OriginalPoints[i].Y;
                        dx *= Scale.X;
                        dy *= Scale.Y;
                        dx *= dx;
                        dy *= dy;
                        if (dx + dy > 1.5)
                        {
                            tempPoint[nCount].X = OriginalPoints[i].X;
                            tempPoint[nCount++].Y = -OriginalPoints[i].Y;
                            PrevAdded = i;
                        }
                    }
                    TransformedPoints = new Point[nCount];
                    for (int i = 0; i < nCount; i++)
                        TransformedPoints[i] = tempPoint[i];
 
                }
            }
            else
            {
                TransformedPoints = new Point[0];
            }
        }

        public Points Points
        {
            get
            {
                return outerPoints;
            }

            set
            {
                outerPoints = value;
            }
        }

        internal Point PointAtIndex(int index)
        {
            Point pt = new Point(0, 0);
            if(OriginalPoints.Length > 0)
            {
                if (index >= OriginalPoints.Length)
                    pt = OriginalPoints[OriginalPoints.Length-1];
                else
                    if (index >= 0)
                        pt = OriginalPoints[index];
            }
            return pt;
        }

        internal bool GetClosestPoint(Point worldPoint, ref Point ResultPoint)
        {
            bool bRes = false;
            if(OriginalPoints != null && OriginalPoints.Length > 0)
            {
                int beg = 0;
                int end = OriginalPoints.Length - 1;
                if(OriginalPoints[beg].X >= worldPoint.X) 
                {
                    ResultPoint = OriginalPoints[beg];
                    return true;
                }

                if(OriginalPoints[end].X <= worldPoint.X)
                {
                    ResultPoint = OriginalPoints[end];
                    return true;
                }

                bRes = true;
                while (end - beg > 1)
                {
                    int mid = beg + (end - beg) / 2;
                    ResultPoint = OriginalPoints[mid];
                    double diff = worldPoint.X - ResultPoint.X;
                    if (diff > 0)
                        beg = mid;
                    else
                        if (diff < 0)
                            end = mid;
                        else
                            break;
                }

                if (Math.Abs(OriginalPoints[beg].X - worldPoint.X) < Math.Abs(OriginalPoints[end].X - worldPoint.X))
                    ResultPoint = OriginalPoints[beg];
                else
                    ResultPoint = OriginalPoints[end];

            }
            return bRes;
        }

        private void FilterRedunantPoints(double Ratio)
        {
            if (OriginalPoints.Length < 3)
                return;
            Point[] FilteredPoints = new Point[OriginalPoints.Length];
            int nCount = 0;
            FilteredPoints[nCount++] = OriginalPoints[0];
            int i = 1;
            while (i < OriginalPoints.Length - 2)
            {

                Point A = FilteredPoints[nCount - 1];
                Point B = OriginalPoints[i];
                Point C = OriginalPoints[i + 1];
                double dX2 = A.X - C.X;
                double dY2 = A.Y - C.Y;
                double dX1 = A.X - B.X;
                double dY1 = A.Y - B.Y;
                if (Math.Abs(dY1) * 10.0 > Math.Abs(dX1) / Ratio)
                {
                    FilteredPoints[nCount++] = OriginalPoints[i];
                }
                else
                {
                    double TriangleBase = Math.Sqrt((dX2 * dX2 + dY2 * dY2));
                    double Area = Math.Abs(A.X * (B.Y - C.Y) + B.X * (C.Y - A.Y) + C.X * (A.Y - B.Y));
                    if (Math.Abs(TriangleBase) > 1E-6)
                    {
                        double Height = Area / TriangleBase;
                        if (Height > TriangleBase * Ratio)
                        {
                            FilteredPoints[nCount++] = OriginalPoints[i];
                        }
                    }
                    else
                    {
                        FilteredPoints[nCount++] = OriginalPoints[i];
                    }
                }
                i++;
            }
            if (nCount > 0)
            {
                FilteredPoints[nCount++] = OriginalPoints[OriginalPoints.Length - 1];
                TransformedPoints = new Point[nCount];
                for (i = 0; i < nCount; i++)
                    TransformedPoints[i] = FilteredPoints[i];

            }
        }

        protected void OnMouseEnter(object sender, EventArgs e)
        {
            legendButton.Highlight = true;
            OnChannelOver(this, new EventOverArgs(true));
        }

        protected void OnMouseLeave(object sender, EventArgs e)
        {
            legendButton.Highlight = false;
            OnChannelOver(this, new EventOverArgs(false));
        }

        protected void OnButtonOver(object sender, EventArgs e)
        {
            EventOverArgs OverArgs = (EventOverArgs)e;
            Highlight = OverArgs.Enter;
            OnChannelOver(this, OverArgs);
        }

        protected void OnButtonClick(object sender, EventArgs e)
        {
            OnChannelClick(this, e);
        }

        protected void OnButtonRightClick(object sender, EventArgs e)
        {
            OnLegendMenu(this, e);
        }

        private void OnChannelOver(object Sender, EventArgs e)
        {
            if (ChannelOver != null)
                ChannelOver(this, e);
        }

        private void OnChannelClick(object Sender, EventArgs e)
        {
            if (ChannelClick != null)
                ChannelClick(this, e);
        }

        private void OnLegendMenu(object Sender, EventArgs e)
        {
            if (LegendMenu != null)
                LegendMenu(this, e);
        }

        private void OnPointsChanged()
        {
            if (PointsChanged != null)
                PointsChanged(this, new EventArgs());
        }

        private void OnAxisChanged(object Sender, EventArgs e)
        {
            if (AxisChanged != null)
                AxisChanged(this, e);
        }

        private void OnPointsChanged(object Sender, EventArgs e)
        {
            PointsEventArgs pe = (PointsEventArgs)e;
            bDirty = true;
            switch(pe.pointChangeType)
            {
                case PointsChangeType.Added:
                if(OriginalPoints == null)
                {
                    OriginalPoints = new Point[1];
                    OriginalPoints[0] = Points.pointAdded;
                }
                else
                {
                    int nLength = OriginalPoints.Length;
                    Point [] tempPoints = new Point[nLength+1];
                    OriginalPoints.CopyTo(tempPoints,0);
                    tempPoints[nLength] = Points.pointAdded;
                    OriginalPoints = tempPoints;
                }
                OnPointsChanged();
                break;

                case PointsChangeType.Cleared:
                    OriginalPoints = null;
                    OnPointsChanged();
                break;

                case PointsChangeType.AddedBulk:
                if (OriginalPoints == null)
                {
                    OriginalPoints = new Point[Points.pointsAdded.Length];
                    Points.pointsAdded.CopyTo(OriginalPoints, 0);
                }
                else
                {
                    int nLength = OriginalPoints.Length;
                    Point[] tempPoints = new Point[nLength + Points.pointsAdded.Length];
                    OriginalPoints.CopyTo(tempPoints, 0);
                    Points.pointsAdded.CopyTo(tempPoints, OriginalPoints.Length);
                    OriginalPoints = tempPoints;
                }
                OnPointsChanged();
                break;
                
                case PointsChangeType.Replaced:
                    OriginalPoints = Points.pointsAdded;
                    OnPointsChanged();
                break;

                case PointsChangeType.Sorted:
                if (OriginalPoints != null)
                {
                    Array.Sort(OriginalPoints, (lhs, rhs) => lhs.X.CompareTo(rhs.X));
                    OnPointsChanged();
                }
                break;
            }
        }

        public Size SetInfoWindowValue(double Yvalue)
        {
            infoWindow.SetValueAndColor(Yvalue, LineColor);
            return infoWindow.GetSize();
        }

        public void UpdateInfoWindow(Point PointerWindow, Point PointerStart, Point PointerEnd, double Yvalue)
        {
            infoWindow.PlacementRectangle = new Rect(PointerWindow.X, PointerWindow.Y, 0, 0);
            infoWindow.Placement = PlacementMode.Center;
            infoWindow.SetValueAndColor(Yvalue, LineColor);
            linePointer.Stroke = new SolidColorBrush(LineColor);
            linePointer.X1 = PointerStart.X;
            linePointer.Y1 = PointerStart.Y;
            linePointer.X2 = PointerEnd.X;
            linePointer.Y2 = PointerEnd.Y;
        }

        public Line InfoWindowPointer
        {
            get { return linePointer;  }
        }

        public List<FrameworkElement> Elements
        {
            get { return elements; }
        }

        public bool InfoWindowOpen
        {
            get { return infoWindow.IsOpen; }
            set { 
                    if (value)
                    {
                        linePointer.Visibility = Visibility.Visible;
                        infoWindow.IsOpen = true;
                    }
                    else
                    {
                        linePointer.Visibility = Visibility.Hidden;
                        infoWindow.IsOpen = false;
                    }
                }
        }
     }
}

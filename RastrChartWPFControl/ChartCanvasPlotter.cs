using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Controls;
using System.Windows.Shapes;
using System.Windows.Input;
using System.Windows.Resources;
using System.Windows.Threading;

namespace RastrChartWPFControl
{
    public class RenderBitmapEventArgs : EventArgs
    {
        public RenderTargetBitmap bmp;
    }

    public class SetReferenceValueEventArgs: EventArgs
    {
        public string ChannelTag;
        public SetReferenceValueEventArgs(string Tag)
        {
            ChannelTag = Tag;
        }
    }

    /*
    public class CSVExportPath : Rastr.AddinSystem.LastUsedPath
    {
        public CSVExportPath() 
            : base("Chart", "CSVExportPath", "CSVData")
        {

        }
    }

    public class PNGExportPath : Rastr.AddinSystem.LastUsedPath
    {
        public PNGExportPath()
            : base("Chart", "PNGExportPath", "PNGimage")
        {

        }
    }
     */

    class EventMarker
    {
        public string m_Name;
        public double m_dTime;

        public EventMarker(string Name, double Time)
        {
            m_Name = Name;
            m_dTime = Time;
        }
    }

    public delegate void SetReferenceValueEvent(SetReferenceValueEventArgs args);
    public delegate void RenderBitmapEvent(RenderBitmapEventArgs args);

    internal class ChartCanvasPlotter
    {
        public event SetReferenceValueEvent SetReferenceValue;
        public event SetReferenceValueEvent ChannelPin;
        public event SetReferenceValueEvent ChannelDelete;
        public event SetReferenceValueEvent ChannelPropsChanged;

        public event EventHandler ObtainFocus;
        public event EventHandler SaveChartSet;

        private Canvas CanvasToPlot;
        private Canvas CanvasToZoom;
        private Canvas CanvasToMeasure;
        private bool IsPanning;
        private bool IsZooming;
        private bool IsInspecting;
        private Point MouseDownPoint;
        private Point Offset;
        private Point ZoomFactor;
        System.Windows.Threading.DispatcherTimer dispatcherTimer;
        Rectangle ZoomRect;
        Line markerYline;
        Line markerXline;
        Line markerTime;

        Cursor ZoomCursor;
        internal List<ChartChannel> Channels;
        DoubleCollection ZoomDash;
        DoubleCollection GridDash;

        private AxisX axisX;
        private AxisYBlock axisYBlock;
        WrapPanel legendArea;

        private bool bInAutoAxes;
        private bool m_bDrawGrid = true;
        private bool m_bDrawLegend = true;
        private bool m_bShowXAxis = false;

        List<HitTestResult> HitTestResults;
        List<EventMarker> EventMarkers = new List<EventMarker>();

        public event EventHandler AskCopyToClipBoard;
        public event RenderBitmapEvent RenderBitmap;

        private double m_dTimeMarker = -1.0;

        private bool bCopyHeadersToClipboard;

        static Color[] standardColors = new Color[16] {Colors.Red,Colors.Blue,Colors.Green,Colors.Magenta,
                                                       Colors.Yellow,Colors.Lime,Colors.Navy,Colors.Orange,
                                                       Colors.Black,Colors.Teal,Colors.Pink,Colors.Gray,
                                                       Colors.Brown,Colors.Olive,Colors.Aqua,Colors.Purple};
            
        private bool IsPan 
        { 
            get
            {
                return IsPanning;
            }

            set
            {
                if (IsPanning != value)
                {
                    IsPanning = value;
                    if (value)
                    {
                        CanvasToPlot.CaptureMouse();
                        CanvasToPlot.Cursor = Cursors.Hand;
                        //RenderOptions.SetEdgeMode(CanvasToPlot,EdgeMode.Aliased);
                    }
                    else
                    {
                        CanvasToPlot.ReleaseMouseCapture();
                        CanvasToPlot.Cursor = Cursors.Arrow;
                        //RenderOptions.SetEdgeMode(CanvasToPlot, EdgeMode.Unspecified);
                    }
                }

            }
        }

        private bool IsZoom 
        {
            get
            {
                return IsZooming;
            }

            set
            {
                if (IsZooming != value)
                {
                    IsZooming = value;
                    if (value)
                    {
                        CanvasToPlot.CaptureMouse();
                        CanvasToZoom.Children.Add(ZoomRect);
                        CanvasToPlot.Cursor = ZoomCursor;

                        if (ZoomFactor.X < 1000.0 && ZoomFactor.Y < 1000.0)
                            CanvasToPlot.CacheMode = new BitmapCache();
                        else
                            CanvasToPlot.CacheMode = null;

                    }
                    else
                    {
                        CanvasToPlot.ReleaseMouseCapture();
                        CanvasToZoom.Children.Remove(ZoomRect);
                        CanvasToPlot.Cursor = Cursors.Arrow;
                        CanvasToPlot.CacheMode = null;
                    }
                }

            }
        }

        private bool IsInspect
        {
            get
            {
                return IsInspecting;
            }

            set
            {
                if (IsInspecting != value)
                {
                    IsInspecting = value;
                    if (value)
                    {
                        CanvasToPlot.CaptureMouse();
                        CanvasToMeasure.Children.Add(markerXline);
                        CanvasToMeasure.Children.Add(markerYline);

                        foreach (ChartChannel channel in Channels)
                            foreach (FrameworkElement elt in channel.Elements)
                                CanvasToMeasure.Children.Add(elt);

                            //CanvasToMeasure.Children.Add(channel.InfoWindowPointer);

                        if (ZoomFactor.X < 1000.0 && ZoomFactor.Y < 1000.0)
                            CanvasToPlot.CacheMode = new BitmapCache();
                        else
                            CanvasToPlot.CacheMode = null;
                    }
                    else
                    {
                        CanvasToPlot.ReleaseMouseCapture();
                        CanvasToMeasure.Children.Remove(markerXline);
                        CanvasToMeasure.Children.Remove(markerYline);

                        foreach (ChartChannel channel in Channels)
                            foreach (FrameworkElement elt in channel.Elements)
                                CanvasToMeasure.Children.Remove(elt);
                        //CanvasToMeasure.Children.Remove(channel.InfoWindowPointer);

                        CanvasToPlot.Cursor = Cursors.Arrow;
                        CanvasToPlot.CacheMode = null;
                        foreach (AxisY axis in axisYBlock.Axes)
                            axis.infoWindow.IsOpen = false;
                        axisX.infoWindow.IsOpen = false;
                        foreach (ChartChannel channel in Channels)
                            channel.InfoWindowOpen = false;
                    }
                }
            }
        }



        public ChartCanvasPlotter(Canvas PlotCanvas, 
                                  Canvas ZoomCanvas,
                                  Canvas MeasureCanvas,
                                  AxisX Xaxis,
                                  AxisYBlock YaxisBlock,
                                  WrapPanel LegendArea)
        {
            CanvasToPlot = PlotCanvas;
            CanvasToZoom = ZoomCanvas;
            CanvasToMeasure = MeasureCanvas;
            RenderOptions.SetEdgeMode(CanvasToZoom, EdgeMode.Aliased);
            axisX = Xaxis;
            axisYBlock = YaxisBlock;
            axisYBlock.TranslationChanged += OnAxisTranslation;
            axisX.TranslationChanged += OnAxisXTranslationChanged;
            axisX.ButtonRightClick += OnAxisXRightClick;
            axisYBlock.ButtonRightClick += OnAxisXRightClick;
            legendArea = LegendArea;
            CanvasToPlot.ClipToBounds = CanvasToMeasure.ClipToBounds = CanvasToZoom.ClipToBounds = true;
            ZoomDash = new DoubleCollection(new double [] {5,5});
            GridDash = new DoubleCollection(new double[] {5,3});
            ZoomDash.Freeze();
            GridDash.Freeze();
            IsPanning = false;
            IsZooming = false;
            bCopyHeadersToClipboard = true;
            Offset.X = Offset.Y = 0.0;
            ZoomFactor.X = ZoomFactor.Y = 1.0;
            CanvasToPlot.SizeChanged += OnCanvasSizeChanged;
            CanvasToPlot.MouseMove += OnMouseMove;
            CanvasToPlot.LostMouseCapture += OnLostMouseCapture;
            CanvasToPlot.MouseWheel += OnMouseWheel;
            CanvasToPlot.MouseDown += OnMouseDown;
            CanvasToPlot.MouseUp   += OnMouseUp;
            //CanvasToPlot.Loaded += OnLoaded;

            dispatcherTimer = new System.Windows.Threading.DispatcherTimer();
            dispatcherTimer.Tick += ScrollTimerTick;
            dispatcherTimer.Interval = new TimeSpan(0, 0, 0, 0, 1);
            dispatcherTimer.Start();

            ZoomRect = new Rectangle();
            ZoomRect.Stroke = Brushes.Black;
            ZoomRect.StrokeDashArray = ZoomDash;
            ZoomRect.StrokeThickness = 1;
            ZoomRect.SnapsToDevicePixels = true;

            markerYline = new Line();
            markerXline = new Line();
            markerYline.Stroke = markerXline.Stroke = Brushes.Black;
            markerYline.StrokeThickness = markerXline.StrokeThickness = 1;
            markerXline.SnapsToDevicePixels = true;
            markerYline.SnapsToDevicePixels = true;
            markerXline.StrokeDashArray = ZoomDash;
            markerYline.StrokeDashArray = ZoomDash;


            markerTime = new Line();
            markerTime.Stroke = new SolidColorBrush(Color.FromRgb(10, 10, 10));
            markerTime.SnapsToDevicePixels = true;
            markerTime.StrokeThickness = 1;

            ZoomCursor = ((TextBlock)CanvasToPlot.Resources["ZoomCursor"]).Cursor;
            Channels = new List<ChartChannel>();
            HitTestResults = new List<HitTestResult>();

            bInAutoAxes = false;
            UpdateAxes();
        }

        protected bool RemoveChannelUtility(ChartChannel channel)
        {
            bool bRes = true;
            channel.AxisChanged -= OnAxisChanged;
            channel.ChannelOver -= OnChannelOver;
            channel.ChannelClick -= OnChannelClick;
            channel.LegendMenu -= OnLegendMenu;
            channel.PointsChanged -= OnPointsChanged;
            legendArea.Children.Remove(channel.legendButton);
            return bRes;
        }

        public void RemoveAllChannels()
        {
            foreach (ChartChannel channel in Channels)
                RemoveChannelUtility(channel);
            Channels.Clear();
            UpdateAxes();
        }

        public void SortChannelsByTags()
        {
            Channels.Sort((Chan1,Chan2) => Chan1.Tag.CompareTo(Chan2.Tag));
            legendArea.Children.Clear();
            foreach (ChartChannel chan in Channels) legendArea.Children.Add(chan.legendButton);
            Transform();
        }


        public bool RemoveChannel(ChartChannel channel)
        {
            bool bRes = RemoveChannelUtility(channel);
            Channels.Remove(channel);
            UpdateAxes();
            return bRes;
        }

        public ChartChannel GetChannelByLegend(string Legend)
        {
            foreach (ChartChannel channel in Channels)
            {
                if (channel.LegendText == Legend)
                    return channel;
            }
            return null;
        }

        public ChartChannel GetChannelByTag(string Tag)
        {
            foreach (ChartChannel channel in Channels)
            {
                if (channel.Tag == Tag)
                    return channel;
            }
            return null;
        }

        public ChartChannel AddChannel(string Legend)
        {
            ChartChannel newChannel = new ChartChannel(this);
            newChannel.LegendText = Legend;
            if (AddChannel(newChannel))
                return newChannel;
            else
                return null;
        }

        private bool AddChannel(ChartChannel channel)
        {
            bool bRes = true;
            int nCount = Channels.Count;
            channel.LineThickness = 2;
            long Hash = Convert.ToInt64(channel.GetHashCode().ToString());
            while (true)
            {
                try
                {
                    channel.Tag = Hash.ToString();
                    break;
                }
                catch(Exception)
                {
                    Hash++;
                }
            }
            if (nCount > 15)
            {
                int LineType = nCount / 16;
                if (LineDashStyles.IndexAvailable(LineType))
                    channel.LineDashStyle = LineType;
                nCount %= 16;
            }
            channel.LineColor = standardColors[nCount % 16];
            Channels.Add(channel);
            legendArea.Children.Add(channel.legendButton);
            channel.AxisChanged += OnAxisChanged;
            channel.ChannelOver += OnChannelOver;
            channel.ChannelClick += OnChannelClick;
            channel.LegendMenu += OnLegendMenu;
            channel.PointsChanged += OnPointsChanged;
            return bRes;
        }

        private void DrawGrid(AxisData x, AxisData y)
        {
            if (m_bDrawGrid)
            {
                try
                {
                    for (double gx = x.actualStart; gx <= x.actualEnd && x.actualEnd > x.actualStart; gx += x.step)
                    {
                        Line gl = new Line();
                        gl.StrokeThickness = 1;
                        gl.SnapsToDevicePixels = true;
                        gl.Stroke = Brushes.LightGray;
                        gl.StrokeDashArray = GridDash;
                        gl.Y1 = 0;
                        gl.Y2 = CanvasToPlot.ActualHeight;
                        gl.X1 = gl.X2 = gx * ZoomFactor.X + Offset.X;
                        CanvasToPlot.Children.Add(gl);
                    }

                    for (double gy = y.actualStart; gy <= y.actualEnd && y.actualEnd > y.actualStart; gy += y.step)
                    {
                        Line gl = new Line();
                        gl.StrokeThickness = 1;
                        gl.SnapsToDevicePixels = true;
                        gl.Stroke = Brushes.LightGray;
                        gl.StrokeDashArray = GridDash;
                        gl.Y1 = gl.Y2 = gy * ZoomFactor.Y + Offset.Y;
                        gl.X1 = 0;
                        gl.X2 = CanvasToPlot.ActualWidth;
                        CanvasToPlot.Children.Add(gl);
                    }
                }
                catch (Exception) { }
            }
        }

        private void UpdateAxes()
        {
            // find which axes should be removed or added
            List<int> axesIdsToAdd = new List<int>();
            foreach (ChartChannel channel in Channels)
            {
                if (!axisYBlock.CheckAxis(channel.Axis))
                    axesIdsToAdd.Add(channel.Axis);
            }
            List<int> axesIdsToRemove = new List<int>();
            foreach (AxisY axis in axisYBlock.Axes)
            {
                bool bRemove = true;
                foreach (ChartChannel channel in Channels)
                {
                    if (axis.Id == channel.Axis)
                    {
                        bRemove = false;
                        break;
                    }
                }
                if (bRemove)
                    axesIdsToRemove.Add(axis.Id);
                axis.Units = "";
            }

            bool bZoom = axisYBlock.OrganizeAxes(axesIdsToAdd, axesIdsToRemove);

            foreach (ChartChannel channel in Channels)
            {
                axisYBlock.GetAxis(channel.Axis).Units = channel.Units;
            }

            if (axisYBlock.Axes.Count == 0)
                axisYBlock.GetAxisData(0);
            

            if (!bInAutoAxes)
            {
                CanvasToPlot.UpdateLayout();
                if (bZoom)
                    ZoomExtents();
                else
                    Transform();
            }
        }

        private List<string> GetUnitsList()
        {
            List<string> ChannelUnits = new List<string>();
            foreach (ChartChannel channel in Channels)
            {
                bool bAdd = true;
                foreach (string str in ChannelUnits)
                {
                    if (str.ToUpper() == channel.Units.ToUpper())
                    {
                        bAdd = false;
                        break;
                    }
                }
                if (bAdd)
                    ChannelUnits.Add(channel.Units);
            }
            return ChannelUnits;
        }

        public void AutoAxes()
        {
            try
            {
                bInAutoAxes = true;
                List<string> ChannelUnits = GetUnitsList();
                foreach (ChartChannel channel in Channels)
                {
                    int nCount = 0;
                    foreach (string str in ChannelUnits)
                    {
                        if (str.ToUpper() == channel.Units.ToUpper())
                        {
                            channel.Axis = nCount;
                            break;
                        }
                        nCount++;
                    }
                }
            }
            finally
            {
                bInAutoAxes = false;
            }
            UpdateAxes();
        }

        public void Flash(bool bHighlight)
        {
            foreach (ChartChannel channel in Channels)
            {
                if (channel.ToFlash)
                    channel.Highlight = bHighlight;
                else
                    channel.Highlight = false;
            }
        }

        public void ZoomExtents()
        {
            foreach (AxisY axis in axisYBlock.Axes)
                axis.bGotBounds = false;

            foreach (ChartChannel channel in Channels)
            {
                AxisY axis = axisYBlock.GetAxis(channel.Axis);
                Rect bounds = channel.Bounds;

                if (m_bShowXAxis)
                {
                    if (bounds.Y < 0 && bounds.Y + bounds.Height < 0) bounds.Height = -bounds.Y;
                    if (bounds.Y > 0 && bounds.Y + bounds.Height > 0) bounds.Y = 0.0;
                }
                bool bBoundsGood = !Double.IsInfinity(bounds.Width) && !Double.IsInfinity(bounds.Height);

                if (!axis.bGotBounds)
                {
                    if (bBoundsGood)
                    {
                        axis.bounds = bounds;
                        axis.bGotBounds = true;
                    }
                }
                else
                {
                    if(bBoundsGood)
                        axis.bounds.Union(channel.Bounds);
                }
            }

            foreach(AxisY axisy in axisYBlock.Axes)
            {
                if (Math.Abs(axisy.bounds.Height) < 1E-5)
                {
                    axisy.bounds.Y -= 0.5;
                    axisy.bounds.Height = 1;
                }

                if (Math.Abs(axisy.bounds.Width) < 1E-5)
                {
                    axisy.bounds.X -= 0.5;
                    axisy.bounds.Width = 1;
                }
            }

            /*
            if (Math.Abs(axisYBlock.Axes[0].bounds.Height) < 1E-5)
            {
                axisYBlock.Axes[0].bounds.Y -= 0.5;
                axisYBlock.Axes[0].bounds.Height = 1;
            }

            if (Math.Abs(axisYBlock.Axes[0].bounds.Width) < 1E-5)
            {
                axisYBlock.Axes[0].bounds.X -= 0.5;
                axisYBlock.Axes[0].bounds.Width = 1;
            }
            */

            Rect Bounds = axisYBlock.Axes[0].bounds;
            Rect BoundsBackup = Bounds;

            try
            {
                // x - axis constraints
                if (axisX.AxisConstraints.ViewStart is double)
                {
                    if (axisX.AxisConstraints.ViewEnd is double)
                    {
                        // both constraints
                        Bounds.X = (double)axisX.AxisConstraints.ViewStart;
                        Bounds.Width = (double)axisX.AxisConstraints.ViewEnd - Bounds.X;
                    }
                    else
                    {
                        // start constraint without end
                        double rightX = Bounds.Width + Bounds.X;
                        Bounds.X = (double)axisX.AxisConstraints.ViewStart;
                        Bounds.Width = rightX - Bounds.X;
                    }
                }
                else
                {
                    if (axisX.AxisConstraints.ViewEnd is double)
                        Bounds.Width = (double)axisX.AxisConstraints.ViewEnd - Bounds.X;
                }
            }
            catch(Exception)
            {
                Bounds = BoundsBackup;
                axisX.AxisConstraints.Reset();
            }
            

            if (CanvasToPlot.ActualWidth > 0)
            {
                ZoomFactor.X = CanvasToPlot.ActualWidth / Bounds.Width;
                Offset.X = -Bounds.X * ZoomFactor.X;
            }

            // y - axis constraints

            BoundsBackup = Bounds;
            Rect axisBoundsBackup = axisYBlock.GetMainAxis().bounds;

            try
            {
                if (axisYBlock.AxisConstraints.ViewStart is double)
                {
                    if (axisYBlock.AxisConstraints.ViewEnd is double)
                    {
                        axisYBlock.GetMainAxis().bounds.Y = Bounds.Y = -(double)axisYBlock.AxisConstraints.ViewEnd;
                        axisYBlock.GetMainAxis().bounds.Height = Bounds.Height = (double)axisYBlock.AxisConstraints.ViewEnd - (double)axisYBlock.AxisConstraints.ViewStart;
                    }
                    else
                        axisYBlock.GetMainAxis().bounds.Height = Bounds.Height = -(double)axisYBlock.AxisConstraints.ViewStart - Bounds.Y;
                }
                else
                { 
                    if (axisYBlock.AxisConstraints.ViewEnd is double)
                    {
                        double rightX = Bounds.Height + Bounds.Y;
                        axisYBlock.GetMainAxis().bounds.Y = Bounds.Y = -(double)axisYBlock.AxisConstraints.ViewEnd;
                        axisYBlock.GetMainAxis().bounds.Height = Bounds.Height = rightX - Bounds.Y;
                    }
                }
            }
            catch(Exception)
            {
                Bounds = BoundsBackup;
                axisYBlock.GetMainAxis().bounds = axisBoundsBackup ;
                axisYBlock.AxisConstraints.Reset();
            }

            if (CanvasToPlot.ActualHeight > 0)
            {
                ZoomFactor.Y = CanvasToPlot.ActualHeight / Bounds.Height; ;
                Offset.Y = -Bounds.Y * ZoomFactor.Y;
            }

            foreach (AxisY axis in axisYBlock.Axes)
            {
                axis.Translation.X = Bounds.Height / axis.bounds.Height;
                axis.Translation.Y = Bounds.Y - axis.bounds.Y * axis.Translation.X;
            }

            Transform();
        }

        private void Transform()
        {

            CanvasToPlot.Children.Clear();
            axisX.SetTransform(ZoomFactor, Offset);

            if (axisYBlock.Axes.Count() > 1)
                axisYBlock.AxisConstraints.Reset();
            else
                axisYBlock.Axes[0].AxisConstraints = axisYBlock.AxisConstraints;

            foreach (AxisY axis in axisYBlock.Axes)
            {
                Point z = new Point(ZoomFactor.X, ZoomFactor.Y * axis.Translation.X);
                Point o = new Point(Offset.X, Offset.Y + axis.Translation.Y * ZoomFactor.Y);
                axis.SetTransform(z, o);
                axis.AxisConstraints = null;
            }

            DrawGrid(axisX.axisData, axisYBlock.Axes[0].axisData);

            foreach (ChartChannel channel in Channels)
            {
                Point z = new Point(ZoomFactor.X, ZoomFactor.Y * axisYBlock.GetAxis(channel.Axis).Translation.X);
                Point o = new Point(Offset.X, Offset.Y + axisYBlock.GetAxis(channel.Axis).Translation.Y * ZoomFactor.Y);
                channel.SetTransform(z, o, CanvasToPlot.ActualWidth, CanvasToPlot.ActualHeight);
                CanvasToPlot.Children.Add(channel.LinePath);
            }

            if (m_dTimeMarker > 0.0)
            {
                markerTime.X1 = markerTime.X2 = m_dTimeMarker * ZoomFactor.X + Offset.X;
                markerTime.Y1 = 0;
                markerTime.Y2 = CanvasToMeasure.ActualHeight;
                CanvasToPlot.Children.Add(markerTime);
                Storyboard sb = new Storyboard();
                DoubleAnimation fadein = new DoubleAnimation(0.1, 1, new Duration(new TimeSpan(0, 0, 0, 0, 300)));
                DoubleAnimation fadeout = new DoubleAnimation(1, 0.1, new Duration(new TimeSpan(0, 0, 0, 1, 0)));
                fadeout.BeginTime = new TimeSpan(0, 0, 0, 0, 300);
                Storyboard.SetTargetProperty(fadein, new PropertyPath("(Line.Opacity)"));
                Storyboard.SetTargetProperty(fadeout, new PropertyPath("(Line.Opacity)"));
                sb.Children.Add(fadein);
                sb.Children.Add(fadeout);
                sb.RepeatBehavior = RepeatBehavior.Forever;
                markerTime.BeginStoryboard(sb);
            }

            foreach(EventMarker marker in EventMarkers)
            {
                Line newMarker = new Line();
                newMarker.Stroke = Brushes.Black;
                newMarker.SnapsToDevicePixels = true;
                newMarker.StrokeThickness = 1;
                newMarker.X1 = newMarker.X2 = Math.Round(marker.m_dTime * ZoomFactor.X + Offset.X);
                newMarker.Y1 = 0;
                newMarker.Y2 = CanvasToMeasure.ActualHeight;
                CanvasToPlot.Children.Add(newMarker);

                Image markerArrow = new Image();
                markerArrow.SnapsToDevicePixels = true;
                markerArrow.Source = ResourceImage("MarkerArrow.png").Source;
                CanvasToPlot.Children.Add(markerArrow);
                Canvas.SetLeft(markerArrow, Math.Round(newMarker.X2 - 2));
                Canvas.SetTop(markerArrow, 0);
                if (marker.m_Name != "")
                {
                    ToolTip tti = new ToolTip();
                    tti.Content = marker.m_Name;
                    markerArrow.ToolTip = tti;
                }
            }
        }

        private void OnCanvasSizeChanged(object s, EventArgs e)
        {
            SizeChangedEventArgs ea = (SizeChangedEventArgs)e;

            if (ea.PreviousSize.Width > 0 && ea.NewSize.Width > 0)
            {
                double ZoomChange = ea.NewSize.Width / ea.PreviousSize.Width;
                ZoomFactor.X *= ZoomChange;
                Offset.X *= ZoomChange;
                
            }
            if (ea.PreviousSize.Height > 0 && ea.NewSize.Height > 0)
            {
                double ZoomChange = ea.NewSize.Height / ea.PreviousSize.Height;
                ZoomFactor.Y *= ZoomChange;
                Offset.Y *= ZoomChange;
            }
            Transform();
        }

        private bool EditChannel(ChartChannel channel)
        {
            bool bRes = true;
            dispatcherTimer.Stop();
            ChannelPropsWindow cpw = new ChannelPropsWindow(channel,axisYBlock);
            Point ScreenPoint = CanvasToPlot.PointToScreen(Mouse.GetPosition(CanvasToPlot));

            ScreenPoint.X -= cpw.Width / 2;
            ScreenPoint.Y -= cpw.Height / 2;

            if (ScreenPoint.X < 0) ScreenPoint.X = 0;
            if (ScreenPoint.Y < 0) ScreenPoint.Y = 0;
            if (ScreenPoint.X + cpw.Width > SystemParameters.WorkArea.Width)
                ScreenPoint.X = SystemParameters.WorkArea.Width - cpw.Width;
            if (ScreenPoint.Y + cpw.Height > SystemParameters.WorkArea.Height)
                ScreenPoint.Y = SystemParameters.WorkArea.Height - cpw.Height;

            cpw.Left = ScreenPoint.X;
            cpw.Top = ScreenPoint.Y;

            if (cpw.ShowDialog() == true)
            {
                Transform();
                if (ChannelPropsChanged != null)
                    ChannelPropsChanged(new SetReferenceValueEventArgs(channel.Tag));
            }

            dispatcherTimer.Start();
            return bRes;
        }

        public static Image ResourceImage(string ResourceName)
        {
            Image img = new Image();
            img.Source = new BitmapImage(new Uri("pack://application:,,,/RastrChartWPFControl;component/Resources/Images/" + ResourceName, UriKind.Absolute));
            return img;
        }

        private void OnMouseDown(object s, EventArgs e)
        {
            TryObtainFocus();
            MouseButtonEventArgs me = (MouseButtonEventArgs)e;
            if ((me.ChangedButton == MouseButton.Left && (Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift)) || 
                 me.ChangedButton == MouseButton.Middle))
            {
                if(!IsPan)
                {
                    MouseDownPoint = me.GetPosition(CanvasToPlot);
                    MouseDownPoint.X -= Offset.X;
                    MouseDownPoint.Y -= Offset.Y;
                    IsPan = true;
                }
            }
            else
            if (me.ChangedButton == MouseButton.Left && (Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl)))
            {
                if (!IsZoom)
                {
                    MouseDownPoint = me.GetPosition(CanvasToPlot);
                    Canvas.SetLeft(ZoomRect, MouseDownPoint.X);
                    Canvas.SetTop(ZoomRect, MouseDownPoint.Y);
                    ZoomRect.Width = ZoomRect.Height = 0.0;
                    IsZoom = true;
                }
            }
            else
            if (me.ChangedButton == MouseButton.Right)
            {
                ContextMenu contextMenu = new ContextMenu();
                MenuItem menuItemZoomExtents = new MenuItem();
                menuItemZoomExtents.Header = CanvasToPlot.Resources["ZoomExtents"];
                menuItemZoomExtents.Icon = ResourceImage("ZoomAll.png");
                menuItemZoomExtents.Click += OnZoomExtents;
                contextMenu.Items.Add(menuItemZoomExtents);

                if(axisX.AxisConstraints.IsSet || axisYBlock.AxisConstraints.IsSet)
                {
                    MenuItem menuItemResetAxes = new MenuItem();
                    menuItemResetAxes.Header = CanvasToPlot.Resources["ResetAxes"];
                    menuItemResetAxes.Icon = ResourceImage("AxisSetupPNG.png");
                    menuItemResetAxes.Click += OnResetAxes;
                    contextMenu.Items.Add(menuItemResetAxes);
                }
                else
                {
                    MenuItem menuItemShowXAxis = new MenuItem();
                    menuItemShowXAxis.Header = CanvasToPlot.Resources["ShowXaxis"];
                    menuItemShowXAxis.IsChecked = m_bShowXAxis;
                    menuItemShowXAxis.Click += OnShowXaxis;
                    contextMenu.Items.Add(menuItemShowXAxis);
                }

                MenuItem menuItemShowGrid = new MenuItem();
                menuItemShowGrid.Header = CanvasToPlot.Resources["ShowGrid"];
                menuItemShowGrid.IsChecked = m_bDrawGrid;
                menuItemShowGrid.Click += OnShowGrid;
                contextMenu.Items.Add(menuItemShowGrid);

                MenuItem menuItemShowLegend = new MenuItem();
                menuItemShowLegend.Header = CanvasToPlot.Resources["ShowLegend"];
                menuItemShowLegend.IsChecked = m_bDrawLegend;
                menuItemShowLegend.Click += OnShowLegend;
                contextMenu.Items.Add(menuItemShowLegend);
                
                DeepCanvasHitTest(me.GetPosition(CanvasToPlot));


                if (Channels.Count > 0)
                {
                    MenuItem menuItemAutoAxes = new MenuItem();
                    menuItemAutoAxes.Header = CanvasToPlot.Resources["AutoAxes"];
                    menuItemAutoAxes.Icon = ResourceImage("AutoAxes.png");
                    menuItemAutoAxes.Click += OnAutoAxes;
                    contextMenu.Items.Add(menuItemAutoAxes);

                    MenuItem menuItemCopy = new MenuItem();
                    menuItemCopy.Header = CanvasToPlot.Resources["CopyToClipboard"];
                    menuItemCopy.Icon = ResourceImage("Copy.png");
                    contextMenu.Items.Add(menuItemCopy);

                    MenuItem menuItemCopyBitmap = new MenuItem();
                    menuItemCopyBitmap.Header = CanvasToPlot.Resources["CopyBitmapToClipboard"];
                    menuItemCopyBitmap.Icon = ResourceImage("CopyImage.png");
                    menuItemCopyBitmap.Click += OnCopyBitmapToClipboard;
                    menuItemCopyBitmap.Tag = null;
                    menuItemCopy.Items.Add(menuItemCopyBitmap);

                    MenuItem menuItemSaveChartSet = new MenuItem();
                    menuItemSaveChartSet.Header = CanvasToPlot.Resources["SaveChartSet"];
                    menuItemSaveChartSet.Icon = ResourceImage("SaveChartSet.png");
                    menuItemSaveChartSet.Click += OnSaveChartSet;
                    contextMenu.Items.Add(menuItemSaveChartSet);


                    GenerateCopyContextMenu(null, menuItemCopy);
                    
                    MenuItem menuItemSaveCSV = new MenuItem();

                    menuItemSaveCSV.Header = CanvasToPlot.Resources["SaveCSV"];
                    menuItemSaveCSV.Icon = ResourceImage("FileIconCSV.png");
                    menuItemSaveCSV.Click += OnSaveDataToCSV;
                    contextMenu.Items.Add(menuItemSaveCSV);

                    MenuItem menuItemSavePNG= new MenuItem();

                    menuItemSavePNG.Header = CanvasToPlot.Resources["SavePNG"];
                    menuItemSavePNG.Icon = ResourceImage("FileIconPNG.png");
                    menuItemSavePNG.Click += OnSaveToPNG;
                    contextMenu.Items.Add(menuItemSavePNG);

                }

                bool bFound = false;
                foreach (HitTestResult hit in HitTestResults)
                {
                    if (hit.VisualHit.GetType() == typeof(Path))
                    {
                        foreach (ChartChannel channel in Channels)
                        {
                            if (channel.LinePath == hit.VisualHit)
                            {
                                MenuItem menuItemChannelGroup = new MenuItem();
                                menuItemChannelGroup.Header = channel.LegendText;
                                contextMenu.Items.Add(menuItemChannelGroup);

                                MenuItem menuItemChannelProps = new MenuItem();
                                menuItemChannelProps.Tag = channel;
                                menuItemChannelProps.Header = CanvasToPlot.Resources["ChannelProperties"];
                                menuItemChannelProps.Icon = ResourceImage("ChanProps.png");
                                menuItemChannelProps.Click += OnChannelProperties;
                                menuItemChannelGroup.Items.Add(menuItemChannelProps);

                                MenuItem menuItemChannelDelete = new MenuItem();
                                menuItemChannelDelete.Tag = channel;
                                menuItemChannelDelete.Header = CanvasToPlot.Resources["DeleteChannel"];
                                menuItemChannelDelete.Icon = ResourceImage("Delete.png");
                                menuItemChannelDelete.Click += OnChannelRemove;
                                menuItemChannelGroup.Items.Add(menuItemChannelDelete);

                                MenuItem menuItemCopyChannel = new MenuItem();
                                menuItemCopyChannel.Header = CanvasToPlot.Resources["CopyToClipboard"];
                                menuItemCopyChannel.Icon = ResourceImage("Copy.png");
                                menuItemChannelGroup.Items.Add(menuItemCopyChannel);

                                GenerateCopyContextMenu(channel,menuItemCopyChannel);

                                try
                                {
                                    MenuItem menuItemSetReference = GenerateSetReferenceMenu((ChartChannel)channel);
                                    if (menuItemSetReference != null)
                                        menuItemChannelGroup.Items.Add(menuItemSetReference);
                                    MenuItem menuItemPin = GeneratePinMenu((ChartChannel)channel);
                                    if (menuItemPin != null)
                                        menuItemChannelGroup.Items.Add(menuItemPin);
                                }
                                catch { };

                                bFound = true;
                                break;
                            }
                        }
                    }
                    if (bFound)
                        break;
                }
                contextMenu.IsOpen = true;
                IsPan = false;
                IsInspect = false;
            }
            else
            if (me.ChangedButton == MouseButton.Left)
            {
                if (!IsInspect)
                {
                    IsInspect = true;
                    PositionInspectCross();
                }
            }
        }

        private MenuItem GenerateSetReferenceMenu(ChartChannel Channel)
        {
            if (Channel.CanBeReference)
            {
                MenuItem menuItemSetAsReference = new MenuItem();
                menuItemSetAsReference.Header = CanvasToPlot.Resources["SetAsReference"];
                menuItemSetAsReference.Icon = ResourceImage("SetAsReference.png");
                menuItemSetAsReference.Tag = Channel;
                menuItemSetAsReference.Click += OnChannelSetReference;
                return menuItemSetAsReference;
            }
            return null;
        }

        private MenuItem GeneratePinMenu(ChartChannel Channel)
        {
            MenuItem menuItemPin = new MenuItem();
            if (Channel.Pin)
            {
                menuItemPin.Header = CanvasToPlot.Resources["UnPin"];
                menuItemPin.Icon = ResourceImage("UnPin.png");
            }
            else
            {
                menuItemPin.Header = CanvasToPlot.Resources["Pin"];
                menuItemPin.Icon = ResourceImage("Pin.png");
            }
            menuItemPin.Tag = Channel;
            menuItemPin.Click += OnChannelPin;
            return menuItemPin;
        }

        private void GenerateCopyContextMenu(object Channel, MenuItem ParentMenu)
        {

            MenuItem menuItemCopyData = new MenuItem();
            menuItemCopyData.Header = CanvasToPlot.Resources["CopyDataToClipboard"];
            menuItemCopyData.Icon = ResourceImage("CopyData.png");
            menuItemCopyData.Click += OnCopyDataToClipboad;
            menuItemCopyData.Tag = Channel;
            ParentMenu.Items.Add(menuItemCopyData);

            MenuItem menuItemCopyDataAndTime = new MenuItem();
            menuItemCopyDataAndTime.Header = CanvasToPlot.Resources["CopyDataAndTimeToClipboard"];
            menuItemCopyDataAndTime.Icon = ResourceImage("CopyDataTime.png");
            menuItemCopyDataAndTime.Click += OnCopyDataAndTimeToClipboad;
            menuItemCopyDataAndTime.Tag = Channel;
            ParentMenu.Items.Add(menuItemCopyDataAndTime);

            MenuItem menuItemCopyHeadersToClipboard = new MenuItem();
            menuItemCopyHeadersToClipboard.Header = CanvasToPlot.Resources["CopyHeadersToClipboard"];
            menuItemCopyHeadersToClipboard.Click += OnCopyHeadersToClipboard;
            menuItemCopyHeadersToClipboard.IsCheckable = true;
            menuItemCopyHeadersToClipboard.IsChecked = bCopyHeadersToClipboard;
            ParentMenu.Items.Add(menuItemCopyHeadersToClipboard);
        }


        protected int DeepCanvasHitTest(Point pt)
        {
            HitTestResults.Clear();
            VisualTreeHelper.HitTest(CanvasToPlot, null, new HitTestResultCallback(HitTestCallback), new PointHitTestParameters(pt));
            return HitTestResults.Count;
        }

        protected HitTestResultBehavior HitTestCallback(HitTestResult result)
        {
            HitTestResults.Add(result);
            return HitTestResultBehavior.Continue;
        }

        protected void OnChannelRemove(object sender, EventArgs e)
        {
            MenuItem mi = (MenuItem)sender;
            ChartChannel channel = (ChartChannel)mi.Tag;
            RemoveChannel(channel);
            if(ChannelDelete != null)
                ChannelDelete(new SetReferenceValueEventArgs(channel.Tag));
        }

        protected void OnChannelPin(object sender, EventArgs e)
        {
            MenuItem mi = (MenuItem)sender;
            ChartChannel channel = (ChartChannel)mi.Tag;
            channel.Pin = !channel.Pin;
            if (ChannelPin != null)
                ChannelPin(new SetReferenceValueEventArgs(channel.Tag));
        }

        protected void OnChannelSetReference(object sender, EventArgs e)
        {
            MenuItem mi = (MenuItem)sender;
            ChartChannel channel = (ChartChannel)mi.Tag;
            if (SetReferenceValue != null)
                SetReferenceValue(new SetReferenceValueEventArgs(channel.Tag));
        }


        protected void OnChannelProperties(object sender, EventArgs e)
        {
            MenuItem mi = (MenuItem)sender;
            ChartChannel channel = (ChartChannel)mi.Tag;
            EditChannel(channel);
        }

        protected void OnAutoAxes(object sender, EventArgs e)
        {
            AutoAxes();
        }

        protected void OnZoomExtents(object sender, EventArgs e)
        {
            ZoomExtents();
        }

        protected void OnResetAxes(object sender, EventArgs e)
        {
            axisX.AxisConstraints.Reset();
            axisYBlock.AxisConstraints.Reset();
            ZoomExtents();
        }

        protected void OnSaveChartSet(object sender, EventArgs e)
        {
            if (SaveChartSet != null)
                SaveChartSet(this, e);
        }
                
        protected void OnShowGrid(object sender, EventArgs e)
        {
            m_bDrawGrid = !m_bDrawGrid;
            Transform();
        }

        protected void OnShowLegend(object sender, EventArgs e)
        {
            m_bDrawLegend = !m_bDrawLegend;
            legendArea.Visibility = m_bDrawLegend?Visibility.Visible:Visibility.Collapsed;
            Transform();
        }

        protected void OnShowXaxis(object sender, EventArgs e)
        {
            m_bShowXAxis = !m_bShowXAxis;
            ZoomExtents();
        }

        private void OnMouseUp(object s, EventArgs e)
        {
            IsPan = false;
            IsInspect = false;
            if (IsZoom)
            {
                Rect ZoomResult = new Rect(Canvas.GetLeft(ZoomRect), Canvas.GetTop(ZoomRect), ZoomRect.Width, ZoomRect.Height);
                if (ZoomResult.Width > 0.0 && ZoomResult.Height > 0.0)
                {
                    double xo = CanvasToPlot.ActualWidth / ZoomResult.Width;
                    double yo = CanvasToPlot.ActualHeight / ZoomResult.Height;
                    ZoomFactor.X *= xo;
                    ZoomFactor.Y *= yo;
                    Offset.X = (Offset.X - ZoomResult.Left) * xo;
                    Offset.Y = (Offset.Y - ZoomResult.Top) * yo;
                    Transform();
                }
                IsZoom = false;
            }
        }

        private void OnMouseMove(object s, EventArgs e)
        {
            MouseEventArgs me = (MouseEventArgs)e;
            Point MouseCurrentPoint = me.GetPosition(CanvasToPlot);
            if (IsZoom)
            {
                PositionZoomRect();
            }
            else
            {
                if (IsPan)
                {
                    Offset.X = MouseCurrentPoint.X - MouseDownPoint.X;
                    Offset.Y = MouseCurrentPoint.Y - MouseDownPoint.Y;
                    Transform();
                }
                if (IsInspect)
                    PositionInspectCross();
            }
        }

        private void PositionInspectCross()
        {
            Point MouseCurrentPoint = Mouse.GetPosition(CanvasToMeasure);
            Point MouseZoomPoint = Mouse.GetPosition(CanvasToZoom);

            if (MouseZoomPoint.X >= 0)
            {
                markerXline.X1 = markerXline.X2 = MouseCurrentPoint.X;
                markerXline.Y1 = 0; markerXline.Y2 = CanvasToMeasure.ActualHeight;
                markerXline.Visibility = Visibility.Visible;
            }
            else
                markerXline.Visibility = Visibility.Collapsed;
            Point wp = CanvasToMeasure.PointToScreen(MouseCurrentPoint);
           
            markerYline.X1 = 0; markerYline.X2 = CanvasToMeasure.ActualWidth;
            markerYline.Y1 = markerYline.Y2 = MouseCurrentPoint.Y;

            axisYBlock.MeasurePoint(wp);
            axisX.MeasurePoint(wp);
            axisX.infoWindow.IsOpen = true;

            Point ptt = PresentationSource.FromVisual(CanvasToZoom).CompositionTarget.TransformFromDevice.Transform(new Point(1, 1));

            Point worldPoint = new Point();
            worldPoint.X = (MouseZoomPoint.X - Offset.X) / ZoomFactor.X;

            Point controlPoint = CanvasToPlot.PointToScreen(new Point(0, 0));
            Rect controlRect = new Rect(CanvasToPlot.PointToScreen(new Point(0, 0)),
                                        CanvasToPlot.PointToScreen(new Point(CanvasToPlot.ActualWidth,CanvasToPlot.ActualHeight)));

            controlPoint.X *= ptt.X;
            controlPoint.Y *= ptt.Y;

            double R = 20.0 * (Channels.Count - 1);
            double AngleStep = 2.0 * Math.PI / (Channels.Count + 1);
            double Angle = -Math.PI / 2;


            ChartMarkerRepel markers = new ChartMarkerRepel(CanvasToMeasure);

            foreach (ChartChannel channel in Channels)
            {
                Point pt = new Point();
                if (channel.GetClosestPoint(worldPoint, ref pt))
                {
                    Point z = new Point(ZoomFactor.X, ZoomFactor.Y * axisYBlock.GetAxis(channel.Axis).Translation.X);
                    Point o = new Point(Offset.X, Offset.Y + axisYBlock.GetAxis(channel.Axis).Translation.Y * ZoomFactor.Y);
                    Point sp = new Point(pt.X * z.X + o.X, -pt.Y * z.Y + o.Y);

                    sp.X += controlPoint.X;
                    sp.Y += controlPoint.Y;

                    sp.X /= ptt.X;
                    sp.Y /= ptt.Y;


                    if (controlRect.Contains(sp))
                        markers.AddWindow(channel, sp, pt.Y);
                    else
                        channel.InfoWindowOpen = false;

                    Angle += AngleStep;
                }
                else
                    channel.InfoWindowOpen = false;
            }

            markers.Arrange(CanvasToMeasure);

            if (CanvasToPlot.CacheMode == null)
                CanvasToPlot.InvalidateVisual();
        }

        private void PositionZoomRect()
        {
            Point MouseCurrentPoint = Mouse.GetPosition(CanvasToPlot);
            var x = Math.Min(MouseCurrentPoint.X, MouseDownPoint.X);
            var y = Math.Min(MouseCurrentPoint.Y, MouseDownPoint.Y);
            ZoomRect.Width = Math.Max(MouseCurrentPoint.X, MouseDownPoint.X) - x;
            ZoomRect.Height = Math.Max(MouseCurrentPoint.Y, MouseDownPoint.Y) - y;
            Canvas.SetLeft(ZoomRect, x);
            Canvas.SetTop(ZoomRect, y);

            if(CanvasToPlot.CacheMode == null)
                CanvasToPlot.InvalidateVisual();
        }

        private void OnLostMouseCapture(object s, EventArgs e)
        {
            if (!CanvasToPlot.IsMouseCaptured)
            {
                IsPan = false;
                IsZoom = false;
            }
        }

        private void OnMouseWheel(object s, EventArgs e)
        {
            MouseWheelEventArgs me = (MouseWheelEventArgs)e;
            Point MouseCurrentPoint = me.GetPosition(CanvasToPlot);
            IsZoom = false;
           
            double ZoomChange = me.Delta>0?1.1:1/1.1;

            ZoomFactor.X *= ZoomChange;
            ZoomFactor.Y *= ZoomChange;
            double dX = MouseCurrentPoint.X - ZoomChange * (MouseCurrentPoint.X - Offset.X);
            double dY = MouseCurrentPoint.Y - ZoomChange * (MouseCurrentPoint.Y - Offset.Y);
            MouseDownPoint.X += Offset.X - dX;
            MouseDownPoint.Y += Offset.Y - dY;
            Offset.X = dX;
            Offset.Y = dY;
            Transform();
            if (IsInspect)
                PositionInspectCross();
        }

        private void ScrollTimerTick(object sender, EventArgs e)
        {
            Point MouseCurrentPoint = Mouse.GetPosition(CanvasToPlot);
            if (IsZoom)
            {
                // scroll X
                if(!ScrollZoomRect(MouseCurrentPoint.X,1.0,true))
                ScrollZoomRect(CanvasToPlot.ActualWidth - MouseCurrentPoint.X,-1.0,true);

                // scroll Y
                if (!ScrollZoomRect(MouseCurrentPoint.Y, 1.0, false))
                    ScrollZoomRect(CanvasToPlot.ActualHeight - MouseCurrentPoint.Y, -1.0, false);
            }
            else
            if (IsInspect)
            {
                // scroll X
                if (!ScrollInspectCross(MouseCurrentPoint.X, 1.0, true))
                    ScrollInspectCross(CanvasToPlot.ActualWidth - MouseCurrentPoint.X, -1.0, true);

                // scroll Y
                if (!ScrollInspectCross(MouseCurrentPoint.Y, 1.0, false))
                    ScrollInspectCross(CanvasToPlot.ActualHeight - MouseCurrentPoint.Y, -1.0, false);
            }
        }

        private bool ScrollZoomRect(double distance, double dir, bool bX)
        {
            if (distance < 0)
            {
                distance = distance < -20 ? -20 : distance;
                if (bX)
                {
                    Offset.X -= dir * distance;
                    MouseDownPoint.X -= dir * distance;
                }
                else
                {
                    Offset.Y -= dir * distance;
                    MouseDownPoint.Y -= dir * distance;
                }
                PositionZoomRect();
                Transform();
                return true;
            }
            return false;
        }

        private bool ScrollInspectCross(double distance, double dir, bool bX)
        {
            if (distance < 0)
            {
                distance = distance < -20 ? -20 : distance;
                if (bX)
                {
                    Offset.X -= dir * distance;
                }
                else
                {
                    Offset.Y -= dir * distance;
                }
                PositionInspectCross();
                Transform();
                return true;
            }
            return false;
        }

        protected void OnLegendMenu(object sender, EventArgs e)
        {
            ContextMenu contextMenu = new ContextMenu();
            MenuItem menuItemChannelDelete = new MenuItem();
            menuItemChannelDelete.Tag = sender;
            menuItemChannelDelete.Header = CanvasToPlot.Resources["DeleteChannel"];
            menuItemChannelDelete.Icon = ResourceImage("Delete.png");
            menuItemChannelDelete.Click += OnChannelRemove;
            contextMenu.Items.Add(menuItemChannelDelete);
            MenuItem menuItemCopy = new MenuItem();
            menuItemCopy.Header = CanvasToPlot.Resources["CopyToClipboard"];
            menuItemCopy.Icon = ResourceImage("Copy.png");
            contextMenu.Items.Add(menuItemCopy);

            GenerateCopyContextMenu(sender, menuItemCopy);
            try
            {
                MenuItem menuItemSetReference = GenerateSetReferenceMenu((ChartChannel)sender);
                if(menuItemSetReference != null)
                    contextMenu.Items.Add(menuItemSetReference);
                MenuItem menuItemPin = GeneratePinMenu((ChartChannel)sender);
                if (menuItemPin != null)
                    contextMenu.Items.Add(menuItemPin);
            }
            catch {};
            contextMenu.IsOpen = true;
        }

        protected void OnPointsChanged(object sender, EventArgs e)
        {
            Transform();   
        }

        protected void OnChannelClick(object sender, EventArgs e)
        {
            ChartChannel channel = (ChartChannel)sender;
            EditChannel(channel);
        }

        protected void OnChannelOver(object sender, EventArgs e)
        {
            EventOverArgs eo = (EventOverArgs)e;
            ChartChannel channel = (ChartChannel)sender;
            axisYBlock.Highlight(channel.Axis, eo.Enter,channel.LineColor);
        }

        protected void OnAxisChanged(object sender, EventArgs e)
        {
            UpdateAxes();
        }

        protected void OnCopyBitmapToClipboard(object sender, EventArgs e)
        {
            CopyBitmapToClipboard();
        }

        protected void OnCopyHeadersToClipboard(object sender, EventArgs e)
        {
            bCopyHeadersToClipboard = !bCopyHeadersToClipboard;
        }

        protected void OnAxisTranslation(object sender, EventArgs e)
        {
            Transform();
        }
        protected void OnAxisXTranslationChanged(object sender, EventArgs e)
        {
            double ZoomChange = axisX.Translation.X;
            ZoomFactor.X *= ZoomChange;
            Point MouseCurrentPoint = Mouse.GetPosition(CanvasToPlot); 
            double dX = MouseCurrentPoint.X - ZoomChange * (MouseCurrentPoint.X - Offset.X);
            MouseDownPoint.X += Offset.X - dX;
            Offset.X = dX;
            Transform();
        }

        /*private void OnLoaded(object s, EventArgs e)
        {
            ZoomExtents();
        }*/

        public void CopyCSVToClipboard(object Channel, bool bCopyTime)
        {
            string tabText, csvText;
            GetDataCSV(Channel, bCopyTime, out tabText, out csvText);
            // Create the container object that will hold both versions of the data.
            System.Windows.DataObject dataObject = new System.Windows.DataObject();

            // Add tab-delimited text to the container object as is.
            dataObject.SetText(tabText);

            // Convert the CSV text to a UTF-8 byte stream before adding it to the container object.
            byte[] bytes = System.Text.Encoding.Default.GetBytes(csvText);
            System.IO.MemoryStream stream = new System.IO.MemoryStream(bytes);
            dataObject.SetData(System.Windows.DataFormats.CommaSeparatedValue, stream);

            // Copy the container object to the clipboard.

            System.Windows.Clipboard.SetDataObject(dataObject, true);


        }

        public void GetDataCSV(object Channel, bool bCopyTime, out string TabText, out string CSVText)
        {
            // Generate both tab-delimited and CSV strings.
            StringBuilder tabbedText = new StringBuilder(); 
            StringBuilder csvText = new StringBuilder(); 

            string ListSeparator = System.Globalization.CultureInfo.CurrentCulture.TextInfo.ListSeparator;
            string Quot = "\"";
            string Tab = "\t";


            ChartChannel CopyChannel = null;
            if(Channel != null)
                CopyChannel = (ChartChannel)Channel;

            if (bCopyTime && bCopyHeadersToClipboard)
            {
                tabbedText.Append(Quot + CanvasToPlot.Resources["TimeHeader"].ToString() + Quot + Tab);
                csvText.Append(Quot + CanvasToPlot.Resources["TimeHeader"].ToString() + Quot + ListSeparator);
            }

            if (bCopyHeadersToClipboard)
            {
                bool bFirst = true;
                foreach (ChartChannel channel in Channels)
                {
                    if (CopyChannel != null && CopyChannel != channel)
                        continue;
                    if (!bFirst)
                    {
                        tabbedText.Append(Tab);
                        csvText.Append(ListSeparator);
                    }
                    bFirst = false; 
                    tabbedText.Append(Quot + channel.LegendButtonText + Quot);
                    csvText.Append(Quot + channel.LegendButtonText + Quot);
                }
                tabbedText.Append(Environment.NewLine);
                csvText.Append(Environment.NewLine);
            }

            if (CopyChannel != null)
            {
                foreach (Point pt in CopyChannel.OriginalPoints)
                {
                    if (bCopyTime)
                    {
                        tabbedText.Append(Quot + pt.X.ToString("E") + Quot + Tab);
                        csvText.Append(Quot + pt.X.ToString("E") + Quot + ListSeparator);
                    }

                    tabbedText.Append(Quot + pt.Y.ToString("E") + Quot + Tab);
                    csvText.Append(Quot + pt.Y.ToString("E") + Quot + ListSeparator);

                    tabbedText.Append(Environment.NewLine);
                    csvText.Append(Environment.NewLine);
                }
            }
            else
            {
                Dictionary<double, int> xset = new Dictionary<double, int>();
                int [] ChannelIndexes = new int[Channels.Count];
                foreach (ChartChannel channel in Channels)
                {
                    foreach (Point pt in channel.OriginalPoints)
                        if(!xset.ContainsKey(pt.X))
                            xset.Add(pt.X, 0);
                }

                foreach (KeyValuePair<double, int> X in xset)
                {
                    bool bFirst = true;
                    double x = X.Key;

                    if (bCopyTime)
                    {
                        tabbedText.Append(Quot + x.ToString("E") + Quot);
                        csvText.Append(Quot + x.ToString("E") + Quot);
                        bFirst = false;
                    }
                    
                    int cn = 0;
                    foreach (ChartChannel channel in Channels)
                    {
                        if (!bFirst)
                        {
                            tabbedText.Append(Tab);
                            csvText.Append(ListSeparator);
                        }
                        bFirst = false;

                        double y = 0.0;
                       
                        Point pts = channel.PointAtIndex(ChannelIndexes[cn]);

                        if (x <= pts.X)
                            y = pts.Y;
                        else
                        {
                            while (x > channel.PointAtIndex(ChannelIndexes[cn]).X)
                            {
                                ChannelIndexes[cn]++;
                                y = channel.PointAtIndex(ChannelIndexes[cn]).Y;
                            }
                        }
                        
                    
                        
                        tabbedText.Append(Quot + y.ToString("E") + Quot);
                        csvText.Append(Quot + y.ToString("E") + Quot);

                        cn++;
                    }
                    tabbedText.Append(Environment.NewLine);
                    csvText.Append(Environment.NewLine);
                }
            }
            TabText = tabbedText.ToString();
            CSVText = csvText.ToString();
        }

        protected void OnCopyDataToClipboad(object sender, EventArgs e)
        {
            MenuItem mi = (MenuItem)sender;
            if (mi != null)           
                CopyCSVToClipboard(mi.Tag, false);
            else
                CopyCSVToClipboard(null, false);
        }

        protected void OnSaveToPNG(object sender, EventArgs e)
        {
            try
            {
                string pngExportPath = "c:\\tmp\\";
                Microsoft.Win32.SaveFileDialog saveDialog = new Microsoft.Win32.SaveFileDialog();

                saveDialog.InitialDirectory = System.IO.Path.GetDirectoryName(pngExportPath);
                saveDialog.AddExtension = true;
                saveDialog.FileName = System.IO.Path.GetFileNameWithoutExtension(pngExportPath);
                saveDialog.Filter = RastrChart.strPngFileMask;
                if (saveDialog.ShowDialog() == true)
                {
                    if (RenderBitmap != null)
                    {
                        RenderBitmapEventArgs rea = new RenderBitmapEventArgs();
                        RenderBitmap(rea);
                        PngBitmapEncoder encoder = new PngBitmapEncoder();
                        BitmapFrame outputFrame = BitmapFrame.Create(rea.bmp);
                        encoder.Frames.Add(outputFrame);
                        using (System.IO.FileStream file = System.IO.File.OpenWrite(saveDialog.FileName))
                        {
                            encoder.Save(file);
                        }
                        pngExportPath = saveDialog.FileName;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        protected void OnSaveDataToCSV(object sender, EventArgs e)
        {
            MenuItem mi = (MenuItem)sender;
            string tabText, csvText;
            if (mi != null)
                GetDataCSV(mi.Tag, true, out tabText, out csvText);
            else
                GetDataCSV(null, true, out tabText, out csvText);

            try
            {
                string csvExportPath = "c:\\tmp\\";
                Microsoft.Win32.SaveFileDialog saveDialog = new Microsoft.Win32.SaveFileDialog();

                saveDialog.InitialDirectory = System.IO.Path.GetDirectoryName(csvExportPath);
                saveDialog.AddExtension = true;
                saveDialog.FileName = System.IO.Path.GetFileNameWithoutExtension(csvExportPath); 
                saveDialog.Filter = "CSV-файл (.csv)|*.csv"; 
                if (saveDialog.ShowDialog() == true)
                {
                    using (System.IO.StreamWriter outputFile = new System.IO.StreamWriter(saveDialog.FileName, false, Encoding.GetEncoding(1251)))
                    {
                        outputFile.Write(csvText);
                        csvExportPath = saveDialog.FileName;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }

        }

        protected void OnCopyDataAndTimeToClipboad(object sender, EventArgs e)
        {
            MenuItem mi = (MenuItem)sender;
            if (mi != null)
                CopyCSVToClipboard(mi.Tag, true);
            else
                CopyCSVToClipboard(null, true);
        }
        

        protected void CopyBitmapToClipboard()
        {
            if (AskCopyToClipBoard != null)
                AskCopyToClipBoard(this, new EventArgs());
        }

        protected void TryObtainFocus()
        {
            if(ObtainFocus != null)
                ObtainFocus(this, new EventArgs());
        }

        public double TimeMarker
        {
            set 
                { 
                    m_dTimeMarker = value;
                    Transform();
                }
        }

        public void AddEventMarker(string Name, double Time)
        {
            bool bJoin = false;
            foreach(EventMarker evt in EventMarkers)
            {
                if(Math.Abs(evt.m_dTime - Time) < 1E-6)
                {
                    bJoin = true;
                    evt.m_Name += System.Environment.NewLine;
                    evt.m_Name += Name;
                }
            }

            if(!bJoin)
                EventMarkers.Add(new EventMarker(Name, Time));

            Transform();
        }

        public void ClearEventMarkers()
        {
            EventMarkers.Clear();
            Transform();
        }

        public void CancelMouse()
        {
            IsInspect = false;
            IsZoom = false;
        }

        protected void OnAxisXRightClick(object sender, EventArgs e)
        {
            OnAxisMenu(sender, e);
        }

        protected void OnAxisMenu(object sender, EventArgs e)
        {
            ContextMenu contextMenu = new ContextMenu();
            MenuItem menuItemAxisSetup = new MenuItem();
            menuItemAxisSetup.Tag = sender;
            menuItemAxisSetup.Header = CanvasToPlot.Resources["AxisSetup"];
            menuItemAxisSetup.Icon = ResourceImage("AxisSetupPNG.png");
            menuItemAxisSetup.Click += OnAxisSetup;
            contextMenu.Items.Add(menuItemAxisSetup);
            contextMenu.IsOpen = true;
        }

        protected void OnAxisSetup(object sender, EventArgs e)
        {
            MenuItem mi = (MenuItem)sender;
            AxisSetup axisSetupForm;
            if (mi.Tag == axisX)
            {
                axisSetupForm = new AxisSetup(axisX, axisX.AxisConstraints, this);
            }
            else
            {
                axisSetupForm = new AxisSetup(axisYBlock.GetMainAxis(), axisYBlock.AxisConstraints, this);
            }

            Point ScreenPoint = CanvasToPlot.PointToScreen(Mouse.GetPosition(CanvasToPlot));
            ScreenPoint.X -= axisSetupForm.Width / 2;
            ScreenPoint.Y -= axisSetupForm.Height / 2;

            if (ScreenPoint.X < 0) ScreenPoint.X = 0;
            if (ScreenPoint.Y < 0) ScreenPoint.Y = 0;
            if (ScreenPoint.X + axisSetupForm.Width > SystemParameters.WorkArea.Width)
                ScreenPoint.X = SystemParameters.WorkArea.Width - axisSetupForm.Width;
            if (ScreenPoint.Y + axisSetupForm.Height > SystemParameters.WorkArea.Height)
                ScreenPoint.Y = SystemParameters.WorkArea.Height - axisSetupForm.Height;

            axisSetupForm.Left = ScreenPoint.X;
            axisSetupForm.Top = ScreenPoint.Y;

            if (axisSetupForm.ShowDialog() == true)
            {
                Transform();
               /* if (ChannelPropsChanged != null)
                    ChannelPropsChanged(new SetReferenceValueEventArgs(channel.Tag));*/
            }

            /*RemoveChannel(channel);
            if (ChannelDelete != null)
                ChannelDelete(new SetReferenceValueEventArgs(channel.Tag));*/
        }

        public AxisConstraints AxisXConstraints
        {
            get { return axisX.AxisConstraints; }
            set {
                    axisX.AxisConstraints = value;
                    ZoomExtents();
                }
        }

        public AxisConstraints AxisYConstraints
        {
            get { return axisYBlock.AxisConstraints; }
            set
            {
                axisYBlock.AxisConstraints = value;
                ZoomExtents();
            }
        }

        public Size PlotSize
        {
            get
            {
                return new Size(CanvasToPlot.ActualWidth, CanvasToPlot.ActualHeight);
            }
        }

    }
}

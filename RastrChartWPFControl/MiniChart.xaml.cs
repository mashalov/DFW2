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
using System.Windows.Media.Effects;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace RastrChartWPFControl
{
    /// <summary>
    /// Interaction logic for MiniChart.xaml
    /// </summary>
    /// 
    public partial class MiniChart : UserControl
    {
        private MiniChannels channels;
        private Point ZoomFactor = new Point(1, 1);
        private Point Offset = new Point(0, 0);

        private double OptimalZoom;

        DoubleCollection GridDash;
        private Point Range;
        List<RulerBase> Rulers = new List<RulerBase>();
        RulerBase rulerDrag = null;
        ControlPoint cpDrag = null;
        private Point mouseDownPoint;
        private bool IsPanning;
        private bool hodographMode;
        private HodographMarkingMode markingMode;
        private Point dragPoint;

        private Cursor AddNode;
        private Cursor RemoveNode;

        public ComplexZoneRelay Zone1 = new ComplexZoneRelay();
        public ComplexZoneRelay Zone2 = new ComplexZoneRelay();

        public RulerPositionHandler RulerPosition;
        public event RangeChangedEventHandler RangeChanged;
        public event EventHandler ObtainFocus;

        protected void OnRangeChanged(object sender, RangeChangedEventArgs e)
        {
            if (RangeChanged != null)
                RangeChanged(sender, e);
        }
      
        public MiniChart()
        {
            channels = new MiniChannels();
            channels.Update += OnUpdate;
            channels.RangeChanged += OnRangeChanged;
            InitializeComponent();
            Graph.SizeChanged += OnGraphSizeChanged;
            GridDash = new DoubleCollection(new double[] { 5, 3 });
            AxisY.MainAxis = true;
            Range.X = Double.NegativeInfinity;
            Range.Y = Double.PositiveInfinity;
            LostMouseCapture += OnLostMouseCapture;
            hodographMode = false;
            IsPanning = false;
            AddNode = ((TextBlock)Graph.Resources["AddNodeCursor"]).Cursor;
            RemoveNode = ((TextBlock)Graph.Resources["RemoveNodeCursor"]).Cursor;

            Zone1.ZoneTypeChanged += OnZoneTypeChanged;
            Zone2.ZoneTypeChanged += OnZoneTypeChanged;
            Zone1.ZoneCombinationChanged += OnZoneCombinationChanged;
            Zone2.ZoneCombinationChanged += OnZoneCombinationChanged;
            Zone1.ZoneParametersChanged += OnZoneParametersChanged;
            Zone2.ZoneParametersChanged += OnZoneParametersChanged;


            /*
            Zone1[0].Type = ZoneType.ZoneTypeEllipse;
            Zone1[3].Type = ZoneType.ZoneTypeEllipse;
            Zone1.ZoneCombinations.Add(new ZoneCombination(true, true, true, true));
             */

            /*
            Zone2[0].Type = ZoneType.ZoneTypeEllipse;
            Zone2[3].Type = ZoneType.ZoneTypeEllipse;
            Zone2.ZoneCombinations.Add(new ZoneCombination(true, true, true, true));
             */
        }

        private bool IsPan
        {
            get
            {
                return IsPanning;
            }

            set
            {
                if (hodographMode)
                {
                    if (IsPanning != value)
                    {
                        IsPanning = value;
                        if (value)
                        {
                            CaptureMouse();
                            Cursor = Cursors.Hand;
                        }
                        else
                        {
                            ReleaseMouseCapture();
                            Cursor = Cursors.Arrow;
                        }
                    }
                }
            }
        }

        private Point ToScreen(Point worldPoint)
        {
            return new Point(worldPoint.X * ZoomFactor.X + Offset.X, -worldPoint.Y * ZoomFactor.Y + Offset.Y);
        }

        private Point ToWorld(Point screenPoint)
        {
            return new Point((screenPoint.X - Offset.X) /  ZoomFactor.X, -(screenPoint.Y - Offset.Y) /  ZoomFactor.Y);
        }

        protected void OnRulerPosition(string Tag, double OldPostion, double NewPosition)
        {
            if (RulerPosition != null)
                RulerPosition(this, new RulerEventArgs(Tag,OldPostion,NewPosition));
        }
        public double RangeBegin
        {
            get
            {
                return Math.Max(Range.X, channels.AvailableRange.X);
            }
            set
            {
                Range.X = value;
                Update();
            }
        }

        public double RangeEnd
        {
            get
            {
                return Math.Min(Range.Y,channels.AvailableRange.Y);
            }
            set
            {
                Range.Y = value;
                Update();
            }
        }

        public double RangeMin
        {
            get
            {
                return channels.AvailableRange.X;
            }
        }

        public double RangeMax
        {
            get
            {
                return channels.AvailableRange.Y;
            }
        }


        public HodographMarkingMode MarkingMode
        {
            get { return markingMode; }
            set
            {
                if (markingMode != value)
                {
                    markingMode = value;
                    Update();
                }
            }
        }

        public bool HodographMode
        {
            get { return hodographMode; }
            set
            {
                hodographMode = value;
                channels.HodographMode = hodographMode;
            }
        }

        protected void OnGraphSizeChanged(object sender, EventArgs e)
        {
            if (hodographMode)
            {
                SizeChangedEventArgs ea = (SizeChangedEventArgs)e;
                double ZoomChangeX = 0.0;
                if (ea.PreviousSize.Width > 0 && ea.NewSize.Width > 0)
                    ZoomChangeX = ea.NewSize.Width / ea.PreviousSize.Width;
                double ZoomChangeY = 0.0;
                if (ea.PreviousSize.Height > 0 && ea.NewSize.Height > 0)
                    ZoomChangeY = ea.NewSize.Height / ea.PreviousSize.Height;

                double ZoomChange = ZoomChangeX;
                if (Math.Abs(ZoomChangeX - 1) < Math.Abs(ZoomChangeY - 1))
                    ZoomChange = ZoomChangeY;

                ZoomFactor.Y = ZoomFactor.X *= ZoomChange;
                Offset.Y *= ZoomChange;
                Offset.X *= ZoomChange;
            }
            Update();
        }

        public MiniChannels Channels
        {
            get { return channels; }
        }

        protected void Update()
        {
            UpdateLegend();
            if (!channels.NoData())
            {
                UpdateTranslations();
                if (ZoomFactor.X == 0 || ZoomFactor.Y == 0)
                    ZoomExtents();
                UpdateAxes();
            }
            UpdateChannels();
            UpdateRulers();
        }


        protected void ZoomExtents()
        {
            channels.Range = Range;
            Rect Bounds = channels.GetBounds();
            ZoomFactor.X = Graph.ActualWidth / Bounds.Width;
            ZoomFactor.Y = Graph.ActualHeight / Bounds.Height;

            if (hodographMode)
            {
                OptimalZoom = ZoomFactor.Y = ZoomFactor.X = Math.Min(ZoomFactor.X, ZoomFactor.Y);
                Offset.X = -Bounds.X * ZoomFactor.X + (Graph.ActualWidth - Bounds.Width * ZoomFactor.X) / 2;
                Offset.Y = -Bounds.Y * ZoomFactor.Y + (Graph.ActualHeight - Bounds.Height * ZoomFactor.Y) / 2;
            }
            else
            {
                Offset.X = -Bounds.X * ZoomFactor.X;
                Offset.Y = -Bounds.Y * ZoomFactor.Y;
            }
        }

        protected void UpdateTranslations()
        {
            if (!hodographMode)
                ZoomExtents();
            else
                channels.Range = Range;
        }

        internal void AlignTextBlock(MiniPoint pt, GridHittest Grid, Color color)
        {
            Rect canvasBounds = new Rect(0, 0, Graph.ActualWidth, Graph.ActualHeight);
            double Angle = Math.Atan2(pt.NormalY, pt.NormalX);

            double bx = pt.X * ZoomFactor.X + Offset.X;
            double by = -pt.Y * ZoomFactor.Y + Offset.Y;

            double x = bx + 5 * Math.Cos(Angle);
            double y = by - 5 * Math.Sin(Angle);

            /*Line l1 = new Line();
            l1.X1 = bx;
            l1.X2 = x;
            l1.Y1 = by;
            l1.Y2 = y;

            l1.StrokeThickness = 1;
            l1.Stroke = Brushes.Black;
            Graph.Children.Add(l1);
            */

            if(!canvasBounds.Contains(new Point(bx,by))) return;

            TextBlock tb = new TextBlock();
            switch (MarkingMode)
            {
                case HodographMarkingMode.Angle:
                    tb.Text = pt.Angle.ToString("G3");
                    break;
                case HodographMarkingMode.Time:
                    tb.Text = pt.T.ToString("G3");
                    break;
            }
            tb.Foreground = new SolidColorBrush(color);
            tb.Foreground.Freeze();

            double fontSize = Math.Max(channels.GetBounds().Width, channels.GetBounds().Height) / 15;
            fontSize *= ZoomFactor.X;
            if (fontSize < 6) return;
            if (fontSize > 12) fontSize = 12;

            tb.FontSize = fontSize;

            tb.Measure(new Size(Double.PositiveInfinity,Double.PositiveInfinity));

            
            double width  = tb.DesiredSize.Width / 2;
            double height = tb.DesiredSize.Height / 2;

            double ddx = 0;
            if (Angle > 0) ddx = height / Math.Tan(Angle); else ddx = -height / Math.Tan(Angle);
            if (ddx > width) ddx = width;
            if(ddx < -width) ddx = -width;

            x -= width;
            x += ddx;

            double ddy = 0;
            if (Math.Abs(Angle) < Math.PI / 2) ddy = width * Math.Tan(Angle); else ddy = -width * Math.Tan(Angle);
            if (ddy > height) ddy = height;
            if (ddy < -height) ddy = -height;

            y -= height;
            y -= ddy;

            if (Grid != null)
            {
                if (Grid.Check(x, y) || Grid.Check(x + width * 2, y) || Grid.Check(x, y + height * 2) || Grid.Check(x + width * 2, y + height * 2))
                    return;
                else
                {
                    Grid.Set(x, y);
                    Grid.Set(x + width *2,y);
                    Grid.Set(x,y+height*2);
                    Grid.Set(x + width * 2, y + height * 2);
                }
            }

            Canvas.SetLeft(tb, x);
            Canvas.SetTop(tb ,y);

            Graph.Children.Add(tb);

            
            Ellipse el = new Ellipse();
            el.Width = el.Height = 7;
            el.Fill = new SolidColorBrush(color);
            el.Fill.Freeze();

            Canvas.SetLeft(el, bx - 3.5);
            Canvas.SetTop(el, by - 3.5);

            Graph.Children.Add(el);
        }

        protected void UpdateChannels()
        {
            Graph.Children.Clear();

            if (channels.NoData())
            {
                Border border = new Border();
                border.Width  = Graph.ActualWidth  * 0.8;
                border.Height = Graph.ActualHeight * 0.8;

                Canvas.SetLeft(border, (Graph.ActualWidth - border.Width) / 2);
                Canvas.SetTop(border, (Graph.ActualHeight - border.Height) / 2);

                border.CornerRadius= new CornerRadius(7);
                border.Background = new SolidColorBrush(Color.FromArgb(50, Colors.Wheat.R, Colors.Wheat.G, Colors.Wheat.B));
                border.BorderBrush = new SolidColorBrush(Colors.DarkRed);
                border.BorderThickness = new Thickness(3);

                TextBlock noDataText = new TextBlock();
                noDataText.Text = "Нет данных для отображения графиков. Для настройки АЛАР с использованием графических форм используется последний активный результат расчета. Выполните расчет или активизируйте один из существующих результатов расчета";
                noDataText.FontSize = 14;
                noDataText.HorizontalAlignment = System.Windows.HorizontalAlignment.Center;
                noDataText.VerticalAlignment = System.Windows.VerticalAlignment.Center;
                noDataText.Height = Double.NaN;
                noDataText.TextWrapping = TextWrapping.WrapWithOverflow;
                border.Padding = new Thickness(10, 3, 10, 3);
                border.Child = noDataText;
                
                Graph.Children.Add(border);
                return;
            }

            DrawGrid(AxisX.axisData, AxisY.axisData);

            if (hodographMode)
            {
                if (ZoomFactor.X > 0)
                {
                    GridHittest Grid = new GridHittest(Graph.ActualWidth, Graph.ActualHeight, 15, 15);

                    foreach (MiniChannel mc in channels)
                    {

                        if (mc.Points.PointsRanged != null)
                        {
                            Graph.Children.Add(mc.GetGodographPath(ZoomFactor, Offset));
                            int nBaseCount = 0;
                            int nCount = 0;

                            int nLast = mc.Points.PointsRanged.Length;
                            if (nLast > 0)
                                AlignTextBlock(mc.Points.PointsRanged[nLast - 1], Grid, mc.Color);

                            bool bFirst = true;
                            foreach (MiniPoint pt in mc.Points.PointsRanged)
                            {
                                if (bFirst)
                                {
                                    AlignTextBlock(pt, Grid, mc.Color);
                                    bFirst = false;
                                }
                                else if (nCount == 0)
                                {
                                    AlignTextBlock(pt, Grid, mc.Color);
                                    nCount = nBaseCount;
                                }
                                if (nCount++ > nBaseCount) nCount = 0;
                            }
                        }
                    }
                }
            }
            else
            {
                foreach (MiniChannel mc in channels)
                    Graph.Children.Add(mc.GetChannelPath(ZoomFactor, Offset));
            }
        }

        protected void UpdateAxes()
        {
            AxisX.SetTransform(ZoomFactor, Offset);
            AxisY.SetTransform(ZoomFactor, Offset);
        }

        protected Rect WorldVisibleRect()
        {
            return new Rect(ToWorld(new Point(0, 0)), ToWorld(new Point(ZoneCanvas.ActualWidth, ZoneCanvas.ActualHeight)));
        }

        protected void UpdateComplexZone(ComplexZoneRelay Relay)
        {
            if (!hodographMode) return;

            TransformGroup tg = new TransformGroup();
            tg.Children.Add(new ScaleTransform(ZoomFactor.X, -ZoomFactor.Y));
            tg.Children.Add(new TranslateTransform(Offset.X, Offset.Y));
            tg.Freeze();

            Rect worldVisibleRect = WorldVisibleRect();

            int ZonesCount = 0;

            for (int i = 0; i < 4; i++)
            {
                ZoneGeometry zoneGeo = Relay[i].ZoneGeometry;
                if (zoneGeo == null) continue;
                ZonesCount++;

                GeometryGroup transformGroup = new GeometryGroup();
                transformGroup.Children.Add(zoneGeo.GetGeometry(worldVisibleRect));
                transformGroup.Transform = tg;
                Path path = new Path();
                path.Data = transformGroup;
                path.Fill = (Relay == Zone1) ? StaticColors.Zone1Brush : StaticColors.Zone2Brush;
                path.Stroke = new SolidColorBrush((Relay == Zone1) ? StaticColors.Zone1Brush.Color : StaticColors.Zone2Brush.Color);
                path.StrokeThickness = 1.5;
                path.StrokeLineJoin = PenLineJoin.Round;
                RulerCanvas.Children.Add(path);
            }


            RectangleGeometry inverseBaseRect = new RectangleGeometry(new Rect(0, 0, ZoneCanvas.ActualWidth, ZoneCanvas.ActualHeight));
            Geometry ResultComination = null;


            int ZoneCombCount = Relay.ZoneCombinations.Count;
            for (int z = 0; z < ZoneCombCount; z++)
            {
                Geometry layer = null;
                Geometry Zone = null;

                ZoneCombination ZoneComb = Relay.ZoneCombinations[z];

                int nCount = 0;
                bool bAllOff = true;

                // check all zones off
                for (int i = 0; i < 4; i++)
                {
                    ZoneGeometry zoneGeo = Relay[i].ZoneGeometry;
                    if (zoneGeo == null) continue;
                    if (ZoneComb.GetZone(i))
                    {
                        bAllOff = false;
                        break;
                    }
                }
                

                for (int i = 0; i < 4 ; i++)
                {
                    ZoneGeometry zoneGeo = Relay[i].ZoneGeometry;
                    if (zoneGeo == null) continue;

                    GeometryGroup transformGroup = new GeometryGroup();
                    transformGroup.Transform = tg;
                    transformGroup.Children.Add(zoneGeo.GetGeometry(worldVisibleRect));
                    GeometryCombineMode combMode = GeometryCombineMode.Intersect;
                    if (!bAllOff)
                    {
                        if (ZoneComb.GetZone(i))
                            Zone = transformGroup;
                        else
                        {
                            CombinedGeometry inverse = new CombinedGeometry(GeometryCombineMode.Exclude, inverseBaseRect, transformGroup);
                            Zone = inverse;
                        }
                    }
                    else
                    {
                        Zone = transformGroup;
                        combMode = GeometryCombineMode.Union;
                    }

                    if (ZonesCount < 2)
                        layer = Zone;
                    else
                    {
                        if (layer == null)
                        {
                            CombinedGeometry comb = new CombinedGeometry();
                            comb.GeometryCombineMode = combMode;
                            comb.Geometry1 = Zone;
                            layer = comb;
                        }
                        else
                        {
                            if (nCount == 1)
                            {
                                ((CombinedGeometry)layer).Geometry2 = Zone;
                            }
                            else
                            {
                                CombinedGeometry comb = new CombinedGeometry();
                                comb.GeometryCombineMode = combMode;
                                comb.Geometry1 = Zone;
                                comb.Geometry2 = layer;
                                layer = comb;
                            }
                        }
                    }
                    if (ZoneCombCount < 2)
                        ResultComination = layer;
                    else
                    {
                        if (ResultComination == null)
                        {
                            CombinedGeometry comb = new CombinedGeometry();
                            comb.GeometryCombineMode = GeometryCombineMode.Union;
                            comb.Geometry1 = layer;
                            ResultComination = comb;
                        }
                        else
                        {
                            if (z == 1)
                            {
                                ((CombinedGeometry)ResultComination).Geometry2 = layer;
                            }
                            else
                            {
                                CombinedGeometry comb = new CombinedGeometry();
                                comb.GeometryCombineMode = GeometryCombineMode.Union;
                                comb.Geometry1 = layer;
                                comb.Geometry2 = ResultComination;
                                ResultComination = comb;
                            }
                        }
                    }
                    nCount++;
                }
            }

            
            Path combPath = new Path();

            combPath.Stroke = new SolidColorBrush((Relay == Zone1) ? StaticColors.ComplexZoneBrush1.Color : StaticColors.ComplexZoneBrush2.Color);
            combPath.StrokeThickness = 3;
            combPath.StrokeLineJoin = PenLineJoin.Round;
            combPath.Data = ResultComination;

            combPath.Fill = (Relay == Zone1) ? StaticColors.ComplexZoneBrush1 : StaticColors.ComplexZoneBrush2;

            combPath.Effect = StaticColors.Shadow;

            RulerCanvas.Children.Add(combPath);

            for (int i = 0; i < 4 ; i++)
            {
                ZoneGeometry zoneGeo = Relay[i].ZoneGeometry;

                if (zoneGeo == null) continue;

                foreach (ControlPoint cp in zoneGeo.ControlPoints)
                    cp.Place(RulerCanvas, ZoomFactor, Offset);
            }
        }

        protected void UpdateRulers()
        {
            RulerCanvas.Children.Clear();
            ZoneCanvas.Children.Clear();

            if (!channels.NoData())
            {
                UpdateComplexZone(Zone2);
                UpdateComplexZone(Zone1);

                MarkerX.Visibility = MarkerY.Visibility = Visibility.Visible;

                if (!Double.IsInfinity(ZoomFactor.Y))
                    foreach (RulerBase ruler in Rulers)
                        ruler.PlaceRuler(RulerCanvas, ZoomFactor, Offset);
            }
            else
                MarkerX.Visibility = MarkerY.Visibility = Visibility.Hidden;
        }

        private void DrawGrid(AxisData x, AxisData y)
        {
            for (double gx = x.actualStart; gx <= x.actualEnd && x.actualEnd > x.actualStart; gx += x.step)
            {
                Line gl = new Line();
                gl.StrokeThickness = 1;
                gl.SnapsToDevicePixels = true;
                gl.Stroke = Brushes.LightGray;
                gl.StrokeDashArray = GridDash;
                gl.Y1 = 0;
                gl.Y2 = Graph.ActualHeight;
                gl.X1 = gl.X2 = gx * ZoomFactor.X + Offset.X;
                Graph.Children.Add(gl);
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
                gl.X2 = Graph.ActualWidth;
                Graph.Children.Add(gl);
            }

            Line thickXaxis = new Line();
            thickXaxis.StrokeThickness = 2;
            thickXaxis.SnapsToDevicePixels = true;
            thickXaxis.Stroke = Brushes.DarkGray;
            thickXaxis.Y1 = 0;
            thickXaxis.Y2 = Graph.ActualHeight;
            thickXaxis.X1 = thickXaxis.X2 = Offset.X;
            Graph.Children.Add(thickXaxis);

            Line thickYaxis = new Line();
            thickYaxis.StrokeThickness = 2;
            thickYaxis.SnapsToDevicePixels = true;
            thickYaxis.Stroke = Brushes.DarkGray;
            thickYaxis.X1 = 0;
            thickYaxis.X2 = Graph.ActualWidth;
            thickYaxis.Y1 = thickYaxis.Y2 = Offset.Y;
            Graph.Children.Add(thickYaxis);
            
        }

        protected void UpdateLegend()
        {
            bool bFirst = true;
            LegendText.Children.Clear();

            if (!channels.NoData())
            {
                foreach (MiniChannel mc in channels)
                {
                    TextBlock legendMc = new TextBlock();
                    if (bFirst)
                        bFirst = false;
                    else
                    {
                        TextBlock comma = new TextBlock();
                        comma.Text = ",";
                        LegendText.Children.Add(comma);
                    }

                    legendMc.Text = mc.Legend;
                    legendMc.Foreground = new SolidColorBrush(mc.Color);
                    LegendText.Children.Add(legendMc);
                }
            }
        }

        protected void OnUpdate(object sender, EventArgs e)
        {
            Update();
        }

        private ZoneHitTestResults ZoneHitTest(Point Position)
        {
            ZoneHitTestResults zoneHitResult = null;
            Rect canvasRect = new Rect(0, 0, RulerCanvas.ActualWidth, RulerCanvas.ActualHeight);
            if (canvasRect.Contains(Position))
            {
                for(int z = 0 ; z < 2 ; z++)
                {
                    ComplexZoneRelay Relay = (z == 0) ? Zone1 : Zone2;
                    for (int i = 0; i < 4; i++)
                    {
                        ZoneGeometry zoneGeo = Relay[i].ZoneGeometry;
                        if (zoneGeo != null)
                        {
                            zoneHitResult = zoneGeo.HitTest(Position, ZoomFactor, Offset);
                            if (zoneHitResult != null)
                                break;
                        }
                    }
                    if (zoneHitResult != null)
                        break;
                }
            }
            if (zoneHitResult != null)
                Cursor = AddNode;
            return zoneHitResult;
        }
        private ControlPoint ControlPointHittest(Point Position)
        {
            ControlPoint bestCP = null;
            Rect canvasRect = new Rect(0, 0, RulerCanvas.ActualWidth, RulerCanvas.ActualHeight);
            if (canvasRect.Contains(Position))
            {
                bool bFirst = true;
                double MinDistance = 0.0;

                for (int z = 0; z < 2; z++)
                {
                    ComplexZoneRelay Relay = (z == 0) ? Zone1 : Zone2;
                    for (int i = 0; i < 4; i++)
                    {
                        ZoneGeometry zoneGeo = Relay[i].ZoneGeometry;
                        if (zoneGeo != null)
                        {
                            foreach (ControlPoint pt in zoneGeo.ControlPoints)
                            {
                                Point screenPoint = ToScreen(pt.Position);
                                double dx = screenPoint.X - Position.X;
                                double dy = screenPoint.Y - Position.Y;
                                double distance = Math.Sqrt(dx * dx + dy * dy);
                                if (distance > ControlPoint.ScreenRadius) continue;
                                if (bFirst)
                                {
                                    MinDistance = distance;
                                    bestCP = pt;
                                    bFirst = false;
                                }
                                else
                                {
                                    if (MinDistance > distance)
                                    {
                                        MinDistance = distance;
                                        bestCP = pt;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (bestCP != null)
                Cursor = Cursors.SizeAll;
            return bestCP;
        }

        private RulerBase RulerHittest(Point Position)
        {
            RulerBase bestRuler = null;
            Rect canvasRect = new Rect(0, 0, RulerCanvas.ActualWidth, RulerCanvas.ActualHeight);
            if (canvasRect.Contains(Position))
            {
                double MinDistance = 0.0;
                foreach (RulerBase ruler in Rulers)
                {
                    if (bestRuler == null)
                    {
                        MinDistance = ruler.GetHitTestDistance(Position);
                        if(MinDistance < 15)
                            bestRuler = ruler;
                    }
                    else
                    {
                        double NewDistance = ruler.GetHitTestDistance(Position);
                        if (NewDistance < MinDistance)
                        {
                            MinDistance = NewDistance;
                            bestRuler = ruler;
                        }
                    }
                }
            }
            if (bestRuler != null)
            {
                if (bestRuler.Orientation == RulerOrientation.RulerHorizontal)
                    Cursor = Cursors.SizeNS;
                else
                    Cursor = Cursors.SizeWE;
            }
            return bestRuler;
        }

        private void ControlMouseMove(object sender, MouseEventArgs e)
        {
            if (IsPan)
            {

                Point mouseCurrentPoint = e.GetPosition(Graph);
                Offset.X = mouseCurrentPoint.X - mouseDownPoint.X;
                Offset.Y = mouseCurrentPoint.Y - mouseDownPoint.Y;
                Update();
            }
            else
            {
                Point Position = e.GetPosition(RulerCanvas);
                if (rulerDrag != null)
                {
                    if (rulerDrag.Orientation == RulerOrientation.RulerHorizontal)
                    {
                        if (Position.Y < 0) Position.Y = 0;
                        if (Position.Y > RulerCanvas.ActualHeight) Position.Y = RulerCanvas.ActualHeight;
                        rulerDrag.Drag(Position, ZoomFactor, Offset);
                    }
                    else
                    {
                        if (Position.X < 0) Position.X = 0;
                        if (Position.X > RulerCanvas.ActualWidth) Position.X = RulerCanvas.ActualWidth;
                        rulerDrag.Drag(Position, ZoomFactor, Offset);
                    }
                }
                else
                if (cpDrag != null)
                {
                    Point newPoint = dragPoint;
                    newPoint.X += Position.X;
                    newPoint.Y += Position.Y;

                    if (Keyboard.IsKeyDown(Key.LeftAlt) && cpDrag.Parent.GetType() != typeof(ZoneEllipse))
                    {
                        Point ptOffset = cpDrag.Position;
                        cpDrag.Position = ToWorld(newPoint);
                        ptOffset.X -= cpDrag.Position.X;
                        ptOffset.Y -= cpDrag.Position.Y;

                        foreach (ControlPoint cp in cpDrag.Parent.ControlPoints)
                        {
                            if (cpDrag != cp)
                            {
                                Point pt = cp.Position;
                                pt.Offset(-ptOffset.X, -ptOffset.Y);
                                cp.Position = pt;
                            }
                        }
                    }
                    else
                        cpDrag.Position = ToWorld(newPoint);
                    UpdateRulers();
                }
            }
            ShowCursor();
        }

        private void ControlMouseLBD(object sender, MouseButtonEventArgs e)
        {
            TryObtainFocus();

            if ((e.ChangedButton == MouseButton.Left && (Keyboard.IsKeyDown(Key.LeftShift)) || Keyboard.IsKeyDown(Key.RightShift)) ||
                 e.ChangedButton == MouseButton.Middle)
            {
                if (!IsPan)
                {
                    mouseDownPoint = e.GetPosition(Graph);
                    mouseDownPoint.X -= Offset.X;
                    mouseDownPoint.Y -= Offset.Y;
                    IsPan = true;
                }
            }
            else
            if(e.ChangedButton == MouseButton.Left)
            {
                Point Position = e.GetPosition(RulerCanvas);

                rulerDrag = RulerHittest(Position);
                if (rulerDrag != null)
                {
                    rulerDrag.BeginDrag(Position);
                    CaptureMouse();
                }
                else
                {
                    cpDrag = ControlPointHittest(Position);
                    if (cpDrag != null)
                    {
                        Point screenPoint = ToScreen(cpDrag.Position);
                        dragPoint.X = screenPoint.X - Position.X;
                        dragPoint.Y = screenPoint.Y - Position.Y;
                        CaptureMouse();
                    }
                    else
                    {
                        ZoneHitTestResults zh = ZoneHitTest(Position);
                        if (zh != null)
                        {
                            ZoneNgon zNgon = (ZoneNgon)zh.HitZone;
                            if (zNgon != null)
                            {
                                cpDrag = zNgon.InsertPoint(zh.HitSegment, ToWorld(zh.HitPoint));
                                dragPoint.X = zh.HitPoint.X - Position.X;
                                dragPoint.Y = zh.HitPoint.Y - Position.Y;
                                CaptureMouse();
                                UpdateRulers();
                            }
                        }
                    }
                }

            }
            else 
            if (e.ChangedButton == MouseButton.Right)
            {
                EndDrag();
                IsPan = false;
                ReleaseMouseCapture();
                ContextMenu contextMenu = new ContextMenu();
                MenuItem menuItemCopyBitmap = new MenuItem();
                menuItemCopyBitmap.Header = Graph.Resources["CopyBitmapToClipboard"];
                menuItemCopyBitmap.Icon = ChartCanvasPlotter.ResourceImage("CopyImage.png");
                menuItemCopyBitmap.Click += OnCopyBitmapToClipboard;
                contextMenu.Items.Add(menuItemCopyBitmap);
                contextMenu.IsOpen = true;
            }

        }

        protected void OnCopyBitmapToClipboard(object sender, EventArgs e)
        {
            CopyUIElementToClipboard(MainGrid);
        }

        private static void CopyUIElementToClipboard(FrameworkElement element)
        {
            double width = element.ActualWidth;
            double height = element.ActualHeight;
            RenderTargetBitmap bmpCopied = new RenderTargetBitmap((int)Math.Round(width), (int)Math.Round(height), 96, 96, PixelFormats.Default);
            DrawingVisual dv = new DrawingVisual();
            using (DrawingContext dc = dv.RenderOpen())
            {
                VisualBrush vb = new VisualBrush(element);
                dc.DrawRectangle(vb, null, new Rect(new Point(), new Size(width, height)));
            }
            bmpCopied.Render(dv);
            Clipboard.SetImage(bmpCopied);
        }

        private void ControlMouseLBU(object sender, MouseButtonEventArgs e)
        {
            EndDrag();
            IsPan = false;
            ReleaseMouseCapture();
        }

        protected void EndDrag()
        {
            rulerDrag = null;
            cpDrag = null;
            OnZoneCombinationChanged(this, new EventArgs());
        }

        protected void ShowCursor()
        {
            Point Position = Mouse.GetPosition(RulerCanvas);
            RulerBase rb = null;
            ControlPoint cp = null;
            ZoneHitTestResults zh = null;
            rb  = RulerHittest(Position);
            if (rb == null)
            {
                cp = ControlPointHittest(Position);
                if(cp == null)
                    zh = ZoneHitTest(Position);
            }
            if (rb == null && cp == null && zh == null)
                Cursor = Cursors.Arrow;
        }

        private void OnLostMouseCapture(object s, EventArgs e)
        {
            EndDrag();
            IsPan = false;
        }

        protected RulerBase GetRulerByTag(string Tag)
        {
            foreach(RulerBase ruler in Rulers)
            {
                if (ruler.Tag == Tag)
                    return ruler;
            }
            return null;
        }

        public void AddRuler(RulerOrientation Orientation, Color RulerColor, string Tag, double InitialValue)
        {
            if(GetRulerByTag(Tag) != null)
                throw new Exception(String.Format("Ruler tag \"{0}\" is already in use", Tag));
            RulerBase newRuler = null;

            switch (Orientation)
            {
                case RulerOrientation.RulerHorizontal:
                    newRuler = new RulerHorizontal(MarkerY);
                    break;
                case RulerOrientation.RulerVertical:
                    newRuler = new RulerVertical(MarkerX);
                    break;
                case RulerOrientation.RulerSpot:
                    newRuler = new RulerSpot();
                    newRuler.RulerAskCoordinates += OnRulerAskCoordinates;
                    break;
            }

            if (newRuler != null)
            {
                newRuler.RulerPosition += OnRulerChanged;
                newRuler.Tag = Tag;
                newRuler.RulerColor = RulerColor;
                newRuler.Position = InitialValue;
                Rulers.Add(newRuler);
                Update();
            }
        }

        public void SetRulerValue(string Tag, double Value)
        {
            RulerBase ruler = GetRulerByTag(Tag);
            if (ruler == null)
                throw new Exception(String.Format("No ruler with tag \"{0}\"", Tag));
            else
            {
                ruler.Position = Value;
                UpdateRulers();
            }
        }

        protected void OnRulerChanged(object sender, RulerEventArgs e)
        {
            OnRulerPosition(e.Tag, e.OldPosition, e.NewPosition);
        }

        private void OnMouseWheel(object s, EventArgs e)
        {
            MouseWheelEventArgs me = (MouseWheelEventArgs)e;
            Point MouseCurrentPoint = me.GetPosition(Graph);

            double ZoomChange = me.Delta > 0 ? 1.1 : 1 / 1.1;

            ZoomFactor.X *= ZoomChange;
            ZoomFactor.Y *= ZoomChange;
            double dX = MouseCurrentPoint.X - ZoomChange * (MouseCurrentPoint.X - Offset.X);
            double dY = MouseCurrentPoint.Y - ZoomChange * (MouseCurrentPoint.Y - Offset.Y);
            mouseDownPoint.X += Offset.X - dX;
            mouseDownPoint.Y += Offset.Y - dY;
            Offset.X = dX;
            Offset.Y = dY;
            Update();
        }

        private void OnRulerAskCoordinates(object sender, RulerEventArgs e)
        {
            if (hodographMode)
            {
                
                Point pt  = new Point();
                e.Coordinates = pt;
                foreach (MiniChannel mc in channels)
                {
                    e.Cancel = true;
                    if(mc.FindTimeCoordinates(e.NewPosition, ref pt))
                    {
                        e.Coordinates = pt;
                        e.Cancel = false;
                        break;
                    }
                }
            }
            else
                e.Cancel = true;
        }

        protected void TryObtainFocus()
        {
            if (ObtainFocus != null)
                ObtainFocus(this, new EventArgs());
        }

        private bool CanvasHittest(Canvas CheckCanvas, MouseButtonEventArgs e)
        {
            bool bRes = false;
            Rect CanvasBound = new Rect(0, 0, CheckCanvas.ActualWidth, CheckCanvas.ActualHeight); 
            if(CanvasBound.Contains(e.GetPosition(CheckCanvas)))
                bRes = true;
            return bRes;
        }

        private void ControlMouseDblClick(object sender, MouseButtonEventArgs e)
        {
            if (CanvasHittest(AxisX, e) || CanvasHittest(AxisY, e))
            {
                if (hodographMode)
                {
                    ZoomExtents();
                    Update();
                }
                else
                {
                    RulerBase ruler = GetRulerByTag("t");
                    if (ruler != null && ruler.Orientation == RulerOrientation.RulerVertical && ZoomFactor.X > 0)
                        ruler.Position = (e.GetPosition(AxisX).X - Offset.X) / ZoomFactor.X;
                }

            }
            else
            if (hodographMode)
            {
                ControlPoint cpTest = ControlPointHittest(e.GetPosition(RulerCanvas));
                if(cpTest != null)
                {
                    if (cpTest.Parent.GetType() == typeof(ZoneNgon))
                    {
                        ZoneNgon zNgon = (ZoneNgon)cpTest.Parent;
                        if (zNgon.RemoveControlPoint(cpTest))
                            UpdateRulers();
                    }
                }
            }
        }

        protected void OnZoneCombinationChanged(object sender, EventArgs e)
        {
            UpdateRulers();
        }

        protected void OnZoneParametersChanged(object sender, EventArgs e)
        {
            UpdateRulers();
        }

        protected void OnZoneTypeChanged(object sender, EventArgs e)
        {
            Zone zone = (Zone)sender;
            if(zone.Type == ZoneType.ZoneTypeOff)
                UpdateRulers(); // remove zone;
            else
            {
                ZoneGeometry zg = zone.ZoneGeometry;
                if (zg.Init(WorldVisibleRect()))
                    UpdateRulers();
            }
        }
    }


    public enum HodographMarkingMode
    {
        Time,
        Angle
    }
}


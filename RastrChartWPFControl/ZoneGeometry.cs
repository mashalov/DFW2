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
    public enum ZoneType
    {
        ZoneTypeOff,
        ZoneTypeEllipse,
        ZoneTypeHalfPlaneSingle,
        ZoneTypeHalfPlane,
        ZoneTypeNgon,
        ZoneTypeNgonTrapezoid,
        ZoneTypeCircle,
        ZoneTypeHalfPlaneCenter
    }

    internal class ZoneHitTestResults
    {
        public ZoneGeometry HitZone;
        public int HitSegment;
        public Point HitPoint;

        public ZoneHitTestResults(ZoneGeometry Zone, int HitSegm, Point HitPt)
        {
            HitZone = Zone;
            HitSegment = HitSegm;
            HitPoint = HitPt;
        }
    }

    public class ControlPointEventArgs : EventArgs
    {
        public bool Cancel  { get; set; }
        public bool UseForceVal { get; set; }
        public Point NewVal { get; set; }
        public Point OldVal { get; set; }
        public Point ForceVal { get; set; }

        public ControlPointEventArgs(Point newVal, Point oldVal)
        {
            NewVal = newVal;
            OldVal = oldVal;
            Cancel = false;
            UseForceVal = false;
        }
    }

    public delegate void ControlPointEventHandler(object sender, ControlPointEventArgs e);

    internal abstract class ZoneGeometry
    {
        protected bool bProgrammaticChange;
        public abstract Geometry GetGeometry(Rect worldVisibleRect);
        public event EventHandler ZoneParametersChanged;
        public static double epsilon = 1E-6;

        public abstract ControlPoint[] ControlPoints
        {
            get;
        }

        public abstract bool PointInZone(Point WorldPoint);

        public virtual ZoneHitTestResults HitTest(Point ScreenPoint, Point ZoomFactor, Point Offset)
        {
            return null;
        }

        public abstract ZoneType Type
        {
            get;
        }

        public virtual bool Init(Rect worldVisibleRect)
        {
            return true;
        }

        public void OnZoneParametersChanged()
        {
            if (ZoneParametersChanged != null)
                ZoneParametersChanged(this, new EventArgs());
        }

        public virtual void CleanUp()
        {
            
        }
   }
    
    internal static class StaticColors
    {
        public static SolidColorBrush CPBrush;
        public static SolidColorBrush CPOutline;
        public static SolidColorBrush ZoneCross;
        public static SolidColorBrush ZoneCrossText;
        public static SolidColorBrush ComplexZoneBrush1;
        public static SolidColorBrush ComplexZoneBrush2;
        public static SolidColorBrush Zone1Brush;
        public static SolidColorBrush Zone2Brush;
        public static SolidColorBrush PowerZoneFillBrush;
        public static SolidColorBrush PowerZoneOutlineBrush;
        public static DropShadowEffect Shadow;

        static StaticColors()
        {
            CPBrush = new SolidColorBrush(Colors.Red);
            CPOutline = new SolidColorBrush(Colors.White);
            ComplexZoneBrush1 = new SolidColorBrush(Colors.Blue);
            ComplexZoneBrush1.Opacity = .05;
            ComplexZoneBrush2 = new SolidColorBrush(Colors.Green);
            ComplexZoneBrush2.Opacity = .05;
            Zone1Brush = new SolidColorBrush(Colors.Red);
            Zone1Brush.Opacity = .3;
            Zone2Brush = new SolidColorBrush(Colors.Gold);
            Zone2Brush.Opacity = .3;

            ZoneCross = new SolidColorBrush(Colors.Aqua);
            ZoneCrossText = new SolidColorBrush(Colors.Aqua);

            Shadow = new DropShadowEffect();
            Shadow.Color = Colors.Black;
            Shadow.ShadowDepth = 2;
            Shadow.Opacity = 1;
            Shadow.Direction = -45;
            Shadow.BlurRadius = 8;
            Shadow.Freeze();

            Zone1Brush.Freeze();
            ComplexZoneBrush1.Freeze();
            ComplexZoneBrush2.Freeze();
            CPBrush.Freeze();
            CPOutline.Freeze();
            ZoneCross.Freeze();
            ZoneCrossText.Freeze();
            PowerZoneFillBrush = new SolidColorBrush(Colors.LightGray);
            PowerZoneOutlineBrush = new SolidColorBrush(Colors.Gray);
            PowerZoneFillBrush.Opacity = .5;
            PowerZoneOutlineBrush.Opacity = .3;
            PowerZoneFillBrush.Freeze();
            PowerZoneOutlineBrush.Freeze();
        }
    }

    internal class ZoneCrossMarker
    {
        private MiniPoint worldInfo;
        private Line LineL = null, LineR = null;
        private MiniChart mc_ = null;
        internal ZoneCrossMarker(MiniPoint WorldInfo)
        {
            worldInfo = WorldInfo;
        }

        public MiniPoint MarkPoint
        {
            get { return worldInfo; }
        }

        public void CleanUp()
        {
            if (LineL != null)
            {
                LineL.ToolTipOpening -= mc_.OnCrossMarkToolTip;
                LineR.ToolTipOpening -= mc_.OnCrossMarkToolTip;
            }
        }

        public void Place(Canvas PlaceCanvas, Point ZoomFactor, Point Offset, MiniChart mc)
        {
            LineL = new Line();
            LineR = new Line();
            mc_ = mc;
            LineL.StrokeThickness = LineR.StrokeThickness = 3;
            LineL.Stroke = LineR.Stroke = StaticColors.ZoneCross;
            LineL.StrokeEndLineCap = LineL.StrokeStartLineCap = PenLineCap.Round;
            LineR.StrokeEndLineCap = LineR.StrokeStartLineCap = PenLineCap.Round;
            PlaceCanvas.Children.Add(LineL);
            PlaceCanvas.Children.Add(LineR);

            double size = 0.5 * ZoomFactor.X;
            size = Math.Min(Math.Max(1.0, size),10.0);

            Point worldPosition = new Point(worldInfo.X * ZoomFactor.X + Offset.X, -worldInfo.Y * ZoomFactor.Y + Offset.Y );
           

            LineL.X1 = worldPosition.X - size;
            LineL.X2 = worldPosition.X + size;
            LineL.Y1 = worldPosition.Y - size;
            LineL.Y2 = worldPosition.Y + size;

            LineR.X1 = worldPosition.X + size;
            LineR.X2 = worldPosition.X - size;
            LineR.Y1 = worldPosition.Y - size;
            LineR.Y2 = worldPosition.Y + size;

            TextBlock text = new TextBlock();
            text.Text = Math.Round(worldInfo.T,4).ToString();
            text.Foreground = StaticColors.ZoneCrossText;

            double fontSize = 3;
            fontSize *= ZoomFactor.X;
            if (fontSize < 6) return;
            if (fontSize > 12) fontSize = 12;
            text.FontSize = fontSize;
            Canvas.SetLeft(text, worldPosition.X + size * 1.1);
            Canvas.SetTop(text, worldPosition.Y - size * 1.1);
            PlaceCanvas.Children.Add(text);

            ToolTip tt = new ToolTip();
            LineL.Tag = this;
            LineR.Tag = this;
            LineL.ToolTip = tt;
            LineR.ToolTip = tt;
            LineL.ToolTipOpening += mc.OnCrossMarkToolTip;
            LineR.ToolTipOpening += mc.OnCrossMarkToolTip;
        }
    }

    internal class ZoneCrossMarkers : List<ZoneCrossMarker>
    {

    }



    internal class ControlPoint
    {
        private Point worldPosition;
        private ZoneGeometry parent;
        public event ControlPointEventHandler PositionChanged;

        internal ControlPoint(Point Point, ZoneGeometry ParentZone)
        {
            worldPosition = Point;
            parent = ParentZone;
        }

        public ControlPoint(ZoneGeometry ParentZone)
        {
            parent = ParentZone;
        }

        public ZoneGeometry Parent
        {
            get { return parent; }
        }

        public void Place(Canvas PlaceCanvas, Point ZoomFactor, Point Offset)
        {
            Ellipse e = new Ellipse();
            e.Width = e.Height = ScreenRadius * 2;
            e.Fill = StaticColors.CPBrush;
            e.StrokeThickness = 2;
            e.Stroke = StaticColors.CPOutline;
            Canvas.SetTop(e,-worldPosition.Y * ZoomFactor.Y + Offset.Y - ScreenRadius);
            Canvas.SetLeft(e, worldPosition.X * ZoomFactor.X + Offset.X - ScreenRadius);
            PlaceCanvas.Children.Add(e);
        }

        internal Point TryPosition
        {
            set
            {
                worldPosition = value;
            }
        }

        public Point Position
        {
            get { return worldPosition; }
            set {
                    if (worldPosition != value)
                    {
                        ControlPointEventArgs args = new ControlPointEventArgs(value,worldPosition);
                        if (PositionChanged != null)
                            PositionChanged(this, args);

                        
                        if (!args.Cancel)
                        {
                            if (args.UseForceVal)
                                worldPosition = args.ForceVal;
                            else
                                worldPosition = value;
                        }
                    }
                }
        }

        public static double ScreenRadius
        {
            get { return 6; }
        }
    }
    
    internal class ZoneHalfPlaneSingle : ZoneGeometry
    {
        protected ControlPoint cp1;
        protected PathFigure halfPlaneFigure = new PathFigure();
        protected PathGeometry halfPlane = new PathGeometry();
        protected Point vcp2 = new Point();
        internal struct DirPoint
        {
            public Point pt;
            public int dir;

            public DirPoint(Point point, int direction)
            {
                this.pt = point;
                this.dir = direction;
            }
        }

        public ZoneHalfPlaneSingle() : base()
        {
            halfPlane.Figures.Add(halfPlaneFigure);
            cp1 = new ControlPoint(this);
            cp1.PositionChanged += OnCPPositionChanged;
        }


        public override void CleanUp()
        {
            cp1.PositionChanged -= OnCPPositionChanged;
            base.CleanUp();
        }

        public override ControlPoint[] ControlPoints
        {
            get { return new ControlPoint[] { cp1 }; }
        }

        static protected bool PointInZone(Point WorldPoint, Point P1, Point P2)
        {
            return (P1.Y - P2.Y) * (WorldPoint.X - P2.X) + (P2.X - P1.X) * (WorldPoint.Y - P2.Y) < 0 ? true : false;
        }

        public override bool PointInZone(Point WorldPoint)
        {
            GetOrthogonalPoint();
            Point vcp1 = cp1.Position;
            return PointInZone(WorldPoint, cp1.Position, vcp2);
        }

        virtual protected void OnCPPositionChanged(object sender, EventArgs e)
        {
            if (bProgrammaticChange) return;
            if (sender == cp1)
                PlaceControlPoint();
            OnZoneParametersChanged();
        }

        protected void PlaceControlPoint()
        {
            bProgrammaticChange = true;
            try
            {
                halfPlaneFigure.StartPoint = cp1.Position;
            }
            finally
            {
                bProgrammaticChange = false;
            }
        }

        public override ZoneType Type
        {
            get { return ZoneType.ZoneTypeHalfPlaneSingle; }
        }

        public double R0
        {
            get { return cp1.Position.X; }
            set
            {
                if (Math.Abs(value - R0) > epsilon)
                {
                    cp1.Position = new Point(value, cp1.Position.Y);
                    PlaceControlPoint();
                    OnZoneParametersChanged();
                }
            }
        }

        public double X0
        {
            get { return cp1.Position.Y; }
            set
            {
                if (Math.Abs(value - X0) > epsilon)
                {
                    cp1.Position = new Point(cp1.Position.X, value);
                    PlaceControlPoint();
                    OnZoneParametersChanged();
                }
            }
        }

        protected bool IntersectLineWithCut(Point Line1, Point Line2, Point Cut1, Point Cut2, ref Point Intersection)
        {
            bool bRes = false;
            if (Cut1 != Cut2 && Line1 != Line2)
            {
                double dxcut = Cut2.X - Cut1.X;
                double dycut = Cut2.Y - Cut1.Y;

                double dxline = Line2.X - Line1.X;
                double dyline = Line2.Y - Line1.Y;

                double t = -1;

                if (dxline != 0)
                {
                    double dyldxl = dyline / dxline;
                    double div = (dycut - dyldxl * dxcut);
                    if (div != 0)
                        t = (Line1.Y - Cut1.Y - dyldxl * (Line1.X - Cut1.X)) / div;
                }
                else
                {
                    if (dxcut != 0)
                        t = (Line1.X - Cut1.X) / dxcut;
                }

                if (t >= 0 && t <= 1)
                {
                    bRes = true;
                    Intersection.X = Cut1.X + dxcut * t;
                    Intersection.Y = Cut1.Y + dycut * t;
                }
            }
            return bRes;
        }

        private void GetOrthogonalPoint()
        {
            vcp2.X = cp1.Position.X + cp1.Position.Y;
            vcp2.Y = cp1.Position.Y - cp1.Position.X;
        }

        public override Geometry GetGeometry(Rect worldVisibleRect)
        {
            GetOrthogonalPoint();
            return HalfPlaneGetGeometry(worldVisibleRect);
        }

        protected Geometry HalfPlaneGetGeometry(Rect worldVisibleRect)
        {
            halfPlaneFigure.Segments.Clear();

            Point InterPoint = new Point();
            List<DirPoint> checkList = new List<DirPoint>();
            if (IntersectLineWithCut(cp1.Position, vcp2, worldVisibleRect.TopLeft, worldVisibleRect.TopRight, ref InterPoint))
                checkList.Add(new ZoneHalfPlane.DirPoint(InterPoint, 0));
            if (IntersectLineWithCut(cp1.Position, vcp2, worldVisibleRect.TopRight, worldVisibleRect.BottomRight, ref InterPoint))
                checkList.Add(new ZoneHalfPlane.DirPoint(InterPoint, 1));
            if (IntersectLineWithCut(cp1.Position, vcp2, worldVisibleRect.BottomRight, worldVisibleRect.BottomLeft, ref InterPoint))
                checkList.Add(new ZoneHalfPlane.DirPoint(InterPoint, 2));
            if (IntersectLineWithCut(cp1.Position, vcp2, worldVisibleRect.BottomLeft, worldVisibleRect.TopLeft, ref InterPoint))
                checkList.Add(new ZoneHalfPlane.DirPoint(InterPoint, 3));

            Point[] baseDirs = { worldVisibleRect.TopRight, worldVisibleRect.BottomRight, worldVisibleRect.BottomLeft, worldVisibleRect.TopLeft };
            if (checkList.Count == 2)
            {
                double Angle = Math.Atan2(vcp2.Y - cp1.Position.Y, vcp2.X - cp1.Position.X) / Math.PI * 180;
                double Angle2 = Math.Atan2(checkList[1].pt.Y - checkList[0].pt.Y, checkList[1].pt.X - checkList[0].pt.X) / Math.PI * 180;

                if (Math.Abs(Angle - Angle2) > 1)
                {
                    DirPoint tmp = checkList[0];
                    checkList[0] = checkList[1];
                    checkList[1] = tmp;
                }

                halfPlaneFigure.StartPoint = checkList[0].pt;
                int side = checkList[0].dir;
                halfPlaneFigure.Segments.Add(new LineSegment(baseDirs[side], true));
                while (true)
                {
                    if (++side > 3) side = 0;
                    if (side == checkList[1].dir)
                    {
                        halfPlaneFigure.Segments.Add(new LineSegment(checkList[1].pt, true));
                        break;
                    }
                    halfPlaneFigure.Segments.Add(new LineSegment(baseDirs[side], true));
                }
                halfPlaneFigure.Segments.Add(new LineSegment(halfPlaneFigure.StartPoint, true));
            }
            else if (checkList.Count < 2)
            {
                Point screenCenter = new Point(worldVisibleRect.Left + worldVisibleRect.Width / 2, worldVisibleRect.Top + worldVisibleRect.Height / 2);
                if ((cp1.Position.Y - vcp2.Y) * (screenCenter.X - vcp2.X) +
                    (vcp2.X - cp1.Position.X) * (screenCenter.Y - vcp2.Y) <= 0)
                {
                    halfPlaneFigure.StartPoint = worldVisibleRect.TopLeft;
                    halfPlaneFigure.Segments.Add(new LineSegment(worldVisibleRect.TopRight, true));
                    halfPlaneFigure.Segments.Add(new LineSegment(worldVisibleRect.BottomRight, true));
                    halfPlaneFigure.Segments.Add(new LineSegment(worldVisibleRect.BottomLeft, true));
                    halfPlaneFigure.Segments.Add(new LineSegment(worldVisibleRect.TopLeft, true));
                }
            }
            return halfPlane;
        }
    }

    internal class ZoneHalfPlaneCenter : ZoneHalfPlane
    {
        public ZoneHalfPlaneCenter() : base()
        {
            Phi = Math.PI;
        }

        protected override void OnCPPositionChanged(object sender, EventArgs e)
        {
            if (bProgrammaticChange) return;
            if (sender == cp1)
            {
                PlaceControlPoints();
            }
            OnZoneParametersChanged();
        }
        
        public override bool PointInZone(Point WorldPoint)
        {
            return false;
        }

        public override ControlPoint[] ControlPoints
        {
            get { return new ControlPoint[] { cp1 }; }
        }


        public override ZoneType Type
        {
            get { return ZoneType.ZoneTypeHalfPlaneCenter; }
        }

        
        protected void PlaceControlPoints()
        {
            bProgrammaticChange = true;
            try
            {
                halfPlaneFigure.StartPoint = cp1.Position;
            }
            finally
            {
                bProgrammaticChange = false;
            }
        }

        public double Phi
        {
            get { return Math.Atan2(cp1.Position.Y, cp1.Position.X) + Math.PI / 2.0; }
            set
            {
                double R = Math.Sqrt(R0 * R0 + X0 * X0);
                if (R < 10) R = 10;
                cp1.Position = new Point(R * Math.Cos(value - Math.PI / 2.0), R * Math.Sin(value - Math.PI / 2.0));
                PlaceControlPoint();
                OnZoneParametersChanged();
            }
        }
    }


    internal class ZoneHalfPlane : ZoneHalfPlaneSingle
    {
        
        protected ControlPoint cp2;
              
        public ZoneHalfPlane() : base()
        {
            cp2 = new ControlPoint(this);
            cp2.PositionChanged += OnCPPositionChanged;
        }
                       
        protected override void OnCPPositionChanged(object sender, EventArgs e)
        {
            if (bProgrammaticChange) return;
            if (sender == cp1)
            {
                PlaceControlPoints();
            }
            else if (sender == cp2)
            {
                PlaceControlPoints();
            }
            OnZoneParametersChanged();
        }

        public override void CleanUp()
        {
            cp2.PositionChanged -= OnCPPositionChanged;
            base.CleanUp();
        }

        public override ControlPoint[] ControlPoints
        {
            get { return new ControlPoint[] { cp1, cp2 }; }
        }

        public override bool Init(Rect worldVisibleRect)
        {
            bool bRes = false;
            if (worldVisibleRect.Width > 0 && worldVisibleRect.Height > 0)
            {
                double x = worldVisibleRect.Left + worldVisibleRect.Width / 2.0;
                double y = worldVisibleRect.Top + worldVisibleRect.Height / 2.0;
                double size = Math.Min(worldVisibleRect.Width, worldVisibleRect.Height);
                size /= 5;
                cp2.Position = new Point(x - size, y - size);
                cp1.Position = new Point(x + size, y + size);
                OnZoneParametersChanged();
                bRes = true;
            }
            return bRes;
        }

        public override bool PointInZone(Point WorldPoint)
        {
            return PointInZone(WorldPoint, cp1.Position, cp2.Position);
        }

        public override ZoneType Type
        {
            get { return ZoneType.ZoneTypeHalfPlane; }
        }

        public double R1
        {
            get { return cp2.Position.X; }
            set
            {

                if (Math.Abs(value - R1) > epsilon)
                {
                    cp2.Position = new Point(value, cp2.Position.Y);
                    PlaceControlPoints();
                    OnZoneParametersChanged();
                }
            }
        }
        
        public double X1
        {
            get { return cp2.Position.Y; }
            set
            {
                if (Math.Abs(value - X1) > epsilon)
                {
                    cp2.Position = new Point(cp2.Position.X, value);
                    PlaceControlPoints();
                    OnZoneParametersChanged();
                }
            }
        }

        public override Geometry GetGeometry(Rect worldVisibleRect)
        {
            vcp2 = cp2.Position;
            return base.HalfPlaneGetGeometry(worldVisibleRect);
        }

        protected void PlaceControlPoints()
        {
            bProgrammaticChange = true;
            try
            {
                halfPlaneFigure.StartPoint = cp1.Position;
            }
            finally
            {
                bProgrammaticChange = false;
            }
        }
    }

    internal class ZoneEllipse : ZoneGeometry
    {
        private EllipseGeometry Ellipse = new EllipseGeometry();
        private ControlPoint cpCenter;
        private ControlPoint cpYRadius;
        private ControlPoint cpXRadius;
        private RotateTransform rotation = new RotateTransform(45);
        private TransformGroup transforGroup = new TransformGroup();
        
        public ZoneEllipse()  : base()
        {
            cpCenter = new ControlPoint(this);
            cpYRadius = new ControlPoint(this);
            cpXRadius = new ControlPoint(this);
        
            transforGroup.Children.Add(rotation);
            Ellipse.Transform = transforGroup;

            Ellipse.Center = new Point(10, 10);

            Ellipse.RadiusX = 10;
            Ellipse.RadiusY = 20;
            PlaceControlPoints();
            bProgrammaticChange = false;
            cpCenter.PositionChanged  += OnCPPositionChanged;
            cpYRadius.PositionChanged += OnCPPositionChanged;
            cpXRadius.PositionChanged += OnCPPositionChanged;   
        }

        public override void CleanUp()
        {
            cpCenter.PositionChanged -= OnCPPositionChanged;
            cpYRadius.PositionChanged -= OnCPPositionChanged;
            cpXRadius.PositionChanged -= OnCPPositionChanged;
            base.CleanUp();
        }

        public double Z
        {
            get {
                    return Ellipse.RadiusX;
                }
            set 
            {
                if (Math.Abs(value - Z) > epsilon)
                {
                    if (value > 0)
                    {
                        double k = K;
                        Ellipse.RadiusX = value;
                        Ellipse.RadiusY = k * value;
                        PlaceControlPoints();
                        OnZoneParametersChanged();
                    }
                }
            }
        }

        public double K
        {
            get { return Ellipse.RadiusY / Ellipse.RadiusX; }
            set 
            {
                if (Math.Abs(value - K) > epsilon)
                {
                    if (value > 0)
                        Ellipse.RadiusY = value * Ellipse.RadiusX;
                    PlaceControlPoints();
                    OnZoneParametersChanged();
                }
            }
        }

        public double R0
        {
            get { return Ellipse.Center.X; }
            set 
            {
                if (Math.Abs(value - R0) > epsilon)
                {
                    Ellipse.Center = new Point(value, X0);
                    PlaceControlPoints();
                    OnZoneParametersChanged();
                }
            }
        }

        public double X0
        {
            get { return Ellipse.Center.Y; }
            set 
            {
                if (Math.Abs(value - X0) > epsilon)
                {
                    Ellipse.Center = new Point(R0, value);
                    PlaceControlPoints();
                    OnZoneParametersChanged();
                }
            }
        }

        public double Phi
        {
            get { return rotation.Angle; }
            set 
            {
                if (Math.Abs(value - Phi) > epsilon)
                {
                    rotation.Angle = value;
                    PlaceControlPoints();
                    OnZoneParametersChanged();
                }
            }
        }


        protected void PlaceControlPoints()
        {
            bProgrammaticChange = true;
            try
            {
                rotation.CenterX   = Ellipse.Center.X;
                rotation.CenterY   = Ellipse.Center.Y;
                cpCenter.Position  = Ellipse.Transform.Transform(Ellipse.Center);
                cpXRadius.Position = Ellipse.Transform.Transform(new Point(Ellipse.RadiusX + Ellipse.Center.X, Ellipse.Center.Y));
                cpYRadius.Position = Ellipse.Transform.Transform(new Point(Ellipse.Center.X, Ellipse.RadiusY + Ellipse.Center.Y));
            }
            finally
            {
                bProgrammaticChange = false;
            }
        }

        private double SetRotationAndGetRadius(Point pt, double OrientAngle)
        {
            double dx = pt.X - cpCenter.Position.X;
            double dy = pt.Y - cpCenter.Position.Y;
            double Angle = Math.Atan2(dy, dx);
            rotation.Angle = (Angle - OrientAngle) / Math.PI * 180;
            return Math.Sqrt(dx * dx + dy * dy);
        }

        protected void OnCPPositionChanged(object sender, EventArgs e)
        {
            OnZoneParametersChanged();
            if (bProgrammaticChange) return;

            if (sender == cpCenter)
            {
                Ellipse.Center = cpCenter.Position;
                PlaceControlPoints();
            }
            else if (sender == cpXRadius)
            {
                Ellipse.RadiusX = SetRotationAndGetRadius(cpXRadius.Position,0);
                PlaceControlPoints();
            }
            else if (sender == cpYRadius)
            {
                Ellipse.RadiusY = SetRotationAndGetRadius(cpYRadius.Position,Math.PI/2);
                PlaceControlPoints();
            }
        }

        public override Geometry GetGeometry(Rect worldVisibleRect)
        {
            return Ellipse; 
        }

        public override ControlPoint[] ControlPoints
        {
            get { return new ControlPoint[] {cpCenter,cpXRadius,cpYRadius}; }
        }

        public override ZoneType Type
        {
            get { return ZoneType.ZoneTypeEllipse; }
        }

        public override bool Init(Rect worldVisibleRect)
        {
            bool bRes = false;
            if (worldVisibleRect.Width > 0 && worldVisibleRect.Height > 0)
            {
                double x = worldVisibleRect.Left + worldVisibleRect.Width / 2.0;
                double y = worldVisibleRect.Top + worldVisibleRect.Height / 2.0;
                rotation.Angle = 0;
                rotation.CenterX = x;
                rotation.CenterY = x;
                Ellipse.Center = new Point(x, y);
                double size = Math.Min(worldVisibleRect.Width, worldVisibleRect.Height);
                size /= 5;
                Ellipse.RadiusX = Ellipse.RadiusY = size;
                PlaceControlPoints();
                bRes = true;
            }
            return bRes;
        }

        public override bool PointInZone(Point WorldPoint)
        {
            double angle = -Math.PI / 180 * Phi;
            double co = Math.Cos(angle);
            double si = Math.Sin(angle);
            double z = Z;

            z *= z;
            double Zsq = z * K * K;

            double r = WorldPoint.X - R0;
            double x = WorldPoint.Y - X0;
            double R1 = r * co - x * si;
            double X1 = x * co + r * si;

            R1 *= R1; X1 *= X1;

            return R1 / z + X1 / Zsq <= 1 ? true : false;
        }
    }

    internal abstract class ZoneNgonBase : ZoneGeometry
    {
        protected PathGeometry ngon = new PathGeometry();
        protected PathFigure ngonFigure = new PathFigure();
        protected List<ControlPoint> cps = new List<ControlPoint>();

        public ZoneNgonBase()
        {
            ngon.Figures.Add(ngonFigure);
        }

        protected bool CheckNgon()
        {
            int nVert = cps.Count;
            bool bIntersect = false;
            for (int i = 0; i < nVert - 2 && !bIntersect; i++)
            {
                double dx1 = cps[i + 1].Position.X - cps[i].Position.X;
                double dy1 = cps[i + 1].Position.Y - cps[i].Position.Y;

                for (int j = i + 1; j < nVert; j++)
                {
                    double dx2, dy2;
                    double x2 = cps[j].Position.X;
                    double y2 = cps[j].Position.Y;

                    if (j == nVert - 1)
                    {
                        dx2 = cps[0].Position.X - cps[j].Position.X;
                        dy2 = cps[0].Position.Y - cps[j].Position.Y;
                    }
                    else
                    {
                        dx2 = cps[j + 1].Position.X - cps[j].Position.X;
                        dy2 = cps[j + 1].Position.Y - cps[j].Position.Y;
                    }

                    double t = dy2 * dx1 - dy1 * dx2;

                    if (t != 0)
                    {
                        double t2 = (dx1 * (cps[i].Position.Y - y2) + dy1 * (x2 - cps[i].Position.X)) / t;

                        double t1 = 0;
                        if (Math.Abs(dx1) > Math.Abs(dy1))
                            t1 = (x2 - cps[i].Position.X + dx2 * t2) / dx1;
                        else
                            t1 = (y2 - cps[i].Position.Y + dy2 * t2) / dy1;

                        if ((i == j - 1) && Math.Abs(t1 - 1) < 1E-3 && Math.Abs(t2) < 1E-3)
                            continue;

                        if ((i == 0) && (j == nVert - 1) && Math.Abs(t1) < 1E-3 && Math.Abs(t2 - 1) < 1E-3)
                            continue;

                        if ((t2 >= 0 && t2 <= 1) && (t1 >= 0 && t1 <= 1))
                        {
                            bIntersect = true;
                            break;
                        }

                    }
                }
            }
            return !bIntersect;
        }

        public override Geometry GetGeometry(Rect worldVisibleRect)
        {
            ngonFigure.Segments.Clear();
            if (cps.Count > 0)
            {
                ngonFigure.StartPoint = cps[0].Position;
                for (int i = 1; i < cps.Count; i++)
                    ngonFigure.Segments.Add(new LineSegment(cps[i].Position, true));
                ngonFigure.Segments.Add(new LineSegment(cps[0].Position, true));
            }
            return ngon;
        }

        public override ZoneHitTestResults HitTest(Point ScreenPoint, Point ZoomFactor, Point Offset)
        {
            ZoneHitTestResults hr = null;
            for (int i = 0; i < cps.Count; i++)
            {
                Point p1 = cps[i].Position;
                Point p2;
                if (i == cps.Count - 1)
                    p2 = cps[0].Position;
                else
                    p2 = cps[i + 1].Position;

                p1.X *= ZoomFactor.X;
                p2.X *= ZoomFactor.X;
                p1.Y *= -ZoomFactor.Y;
                p2.Y *= -ZoomFactor.Y;
                p1.X += Offset.X;
                p2.X += Offset.X;
                p1.Y += Offset.Y;
                p2.Y += Offset.Y;

                double dx = p2.X - p1.X;
                double dy = p2.Y - p1.Y;

                double t = -1;
                if (dx == 0)
                {
                    if (dy != 0)
                        t = (ScreenPoint.Y - p1.Y) / dy;
                }
                else
                {
                    t = (dx * (ScreenPoint.X - p1.X) + dy * (ScreenPoint.Y - p1.Y)) / (dx * dx + dy * dy);
                }

                if (t >= 0 && t <= 1)
                {
                    dx = p1.X + dx * t;
                    dy = p1.Y + dy * t;

                    Point HitPt = new Point(dx, dy);

                    dx -= ScreenPoint.X;
                    dy -= ScreenPoint.Y;

                    if (dx * dx + dy * dy < 3)
                        hr = new ZoneHitTestResults(this, i, HitPt);
                }

            }
            return hr;
        }

        public override bool PointInZone(Point WorldPoint)
        {
            int nVert = cps.Count();
            bool bRes = false;
            for (int i = 0, j = nVert - 1; i < nVert; j = i++)
            {
                if (((cps[i].Position.Y >= WorldPoint.Y) != (cps[j].Position.Y >= WorldPoint.Y)) &&
                    (WorldPoint.X <= (cps[j].Position.X - cps[i].Position.X) * (WorldPoint.Y - cps[i].Position.Y) / (cps[j].Position.Y - cps[i].Position.Y) + cps[i].Position.X))
                    bRes = !bRes;
            }
            return bRes;
        }

    }


    internal class ZoneNgon : ZoneNgonBase
    {
        public ZoneNgon() : base ()
        {
    
        }

        protected void OnCPPositionChanged(object sender, ControlPointEventArgs e)
        {
            OnZoneParametersChanged();

            if (bProgrammaticChange) return;
            ControlPoint findPoint = null;
            foreach (ControlPoint cp in cps)
                if (cp == sender)
                {
                    findPoint = cp;
                    break;
                }
            if (findPoint != null)
            {
                findPoint.TryPosition = e.NewVal;
                if (!CheckNgon())
                {
                    findPoint.TryPosition = e.OldVal;
                    e.Cancel = true;
                }
            }
        }
            
        public override ControlPoint[] ControlPoints
        {
            get 
            { 
                ControlPoint[] retcps = new ControlPoint[cps.Count];
                for (int i = 0; i < cps.Count; i++)
                    retcps[i] = cps[i];
                return retcps; 
            }
        }

        public override void CleanUp()
        {
            foreach (var point in cps)
                point.PositionChanged -= OnCPPositionChanged;
            base.CleanUp();
        }

        public bool RemoveControlPoint(ControlPoint ControlPointToRemove)
        {
            bool bRes = false;
            if (cps.Count > 3)
            {
                int nIndex = cps.IndexOf(ControlPointToRemove);
                bRes = cps.Remove(ControlPointToRemove);
                if (bRes)
                {
                    if (!CheckNgon())
                    {
                        cps.Insert(nIndex, ControlPointToRemove);
                        bRes = false;
                    }
                    else
                        ControlPointToRemove.PositionChanged -= OnCPPositionChanged;
                }
            }
            return bRes;
        }

        public ControlPoint InsertPoint(int Segment, Point newPoint)
        {
            ControlPoint newCP = new ControlPoint(newPoint,this);
            newCP.PositionChanged += OnCPPositionChanged;
            cps.Insert(Segment + 1, newCP);
            return newCP;
        }
                
        public override bool Init(Rect worldVisibleRect)
        {
            bool bRes = false;
            if (worldVisibleRect.Width > 0 && worldVisibleRect.Height > 0)
            {
                foreach (ControlPoint cp in cps)
                    cp.PositionChanged -= OnCPPositionChanged;
                cps.Clear();
                Point center = new Point(worldVisibleRect.Left + worldVisibleRect.Width / 2.0, worldVisibleRect.Top + worldVisibleRect.Height / 2.0);
                double size = Math.Min(worldVisibleRect.Width, worldVisibleRect.Height);
                size /= 5;
                cps.Add(new ControlPoint(new Point(center.X - size, center.Y - size),this));
                cps.Add(new ControlPoint(new Point(center.X - size, center.Y + size), this));
                cps.Add(new ControlPoint(new Point(center.X + size, center.Y + size), this));
                cps.Add(new ControlPoint(new Point(center.X + size, center.Y - size), this));

                foreach (ControlPoint cp in cps)
                    cp.PositionChanged += OnCPPositionChanged;

                OnZoneParametersChanged();

                bRes = true;
            }
            return bRes;
        }

        public bool SetControlPoints(Point[] points)
        {
            bool bRes = false;

            if(points.Length > 2)
            {
                List<ControlPoint> cpsOld = new List<ControlPoint>(cps);
                cps.Clear();
                for (int i = 0; i < points.Length; i++)
                    cps.Add(new ControlPoint(points[i],this));

                bRes = CheckNgon();

                if(bRes)
                {
                    foreach (ControlPoint cp in cps)
                        cp.PositionChanged += OnCPPositionChanged;
                    foreach (ControlPoint cp in cpsOld)
                        cp.PositionChanged -= OnCPPositionChanged;
                    cpsOld = null;
                }
                else
                    cps = cpsOld;
            }
            return bRes;
        }

        public override ZoneType Type
        {
            get { return ZoneType.ZoneTypeNgon; }
        }
    }

    internal class ZoneCircle : ZoneGeometry
    {
        private ControlPoint cpZr;
        private ControlPoint cpZo;
        private double m_Zr, m_Zo, m_Phi;
        private EllipseGeometry Ellipse = new EllipseGeometry();

        public ZoneCircle() : base()
        {
            cpZr = new ControlPoint(this);
            cpZo = new ControlPoint(this);
            Zr = 10;
            Zo = -5;
            PlaceControlPoints();
            bProgrammaticChange = false;
            cpZr.PositionChanged += OnCPPositionChanged;
            cpZo.PositionChanged += OnCPPositionChanged;
        }

        public override void CleanUp()
        {
            cpZr.PositionChanged -= OnCPPositionChanged;
            cpZo.PositionChanged -= OnCPPositionChanged;
            base.CleanUp();
        }
        public double Zr
        {
            get { return m_Zr; }
            set
            {
                m_Zr = value;
                PlaceControlPoints();
                OnZoneParametersChanged();
            }
        }

        public double Zo
        {
            get { return m_Zo; }
            set
            {
                m_Zo = value;
                PlaceControlPoints();
                OnZoneParametersChanged();
            }
        }

        public double Phi
        {
            get { return m_Phi; }
            set
            {
                m_Phi = value;
                PlaceControlPoints();
                OnZoneParametersChanged();
            }
        }
              

        protected void PlaceControlPoints()
        {
            bProgrammaticChange = true;
            try
            {
                cpZr.Position = new Point(m_Zr * Math.Cos(m_Phi), m_Zr * Math.Sin(m_Phi));
                cpZo.Position = new Point(m_Zo * Math.Cos(m_Phi), m_Zo * Math.Sin(m_Phi));
                Ellipse.RadiusX = Ellipse.RadiusY = (m_Zr - m_Zo) / 2.0;
                double Rcenter = (m_Zr + m_Zo) * 0.5;
                Ellipse.Center = new Point(Rcenter * Math.Cos(m_Phi), Rcenter * Math.Sin(m_Phi));
            }
            finally
            {
                bProgrammaticChange = false;
            }
        }

        protected void OnCPPositionChanged(object sender, ControlPointEventArgs e)
        {
            OnZoneParametersChanged();
            if (bProgrammaticChange) return;
            if (sender == cpZr)
            {
                m_Phi = Math.Atan2(cpZr.Position.Y, cpZr.Position.X);
                m_Zr = Math.Sqrt(cpZr.Position.X * cpZr.Position.X + cpZr.Position.Y * cpZr.Position.Y);
                PlaceControlPoints();
            }
            else if (sender == cpZo)
            {

                double x = e.NewVal.X;
                double y = e.NewVal.Y;

                // the point of mouse x,y is projected to m_Phi vector, to smooth out
                // control point movement
                // m_Phi vector is given by x(t) = t*cos(phi), y(t) = t*sin(phi)
                // we have to find its line crossing with
                // x(t2) = x + t2 * cos(m_Phi + 90), y(t2) = y + t2 * sin(m_Phi + 90)

                double t2 = x * Math.Sin(m_Phi) - y * Math.Cos(m_Phi);
                x = x - t2 * Math.Sin(m_Phi);
                y = y + t2 * Math.Cos(m_Phi);
                
                // here we try to determine sign of m_Zo
                // m_Zo is computed by square, so it is positive, but depending on control
                // point posiition m_Zo can be negative. To check position we write parametric
                // equation of line, which control point (x0, y0) must belong to m_Phi vector
                // and the find t to minimize f(t) = (x0 - t*cos(phi))^2 + (y0 - t*sin(phi))^2
                // df(t)/dt = 2*t - 2*x0*cos(phi) - 2*y0*sin(phi)
                // happily - there is the only solution





                double t = x * Math.Cos(m_Phi) + y * Math.Sin(m_Phi);
                m_Zo = Math.Sqrt(x * x + y * y);
                if(t < 0)
                    m_Zo = -m_Zo;
                PlaceControlPoints();
                e.UseForceVal = true;
                e.ForceVal = cpZo.Position;
            }
        }

        public override Geometry GetGeometry(Rect worldVisibleRect)
        {
            return Ellipse;
        }

        public override ControlPoint[] ControlPoints
        {
            get { return new ControlPoint[] { cpZr, cpZo }; }
        }

        public override ZoneType Type
        {
            get { return ZoneType.ZoneTypeCircle; }
        }

        public override bool PointInZone(Point WorldPoint)
        {
            return false;
        }
    }
    

    internal class ZoneNgonTrapezoid : ZoneNgonBase
    {
        private ControlPoint cpCenter;
        private ControlPoint cpHAngle;
        private ControlPoint cpSideTop;
        private ControlPoint cpSideBot;

        private double m_H, m_LTop, m_LBot, m_Alpha;

        public ZoneNgonTrapezoid() : base()
        {
            cps.Add(new ControlPoint(new Point(-10, 10), this));
            cps.Add(new ControlPoint(new Point(10, 10), this));
            cps.Add(new ControlPoint(new Point(10, -10), this));
            cps.Add(new ControlPoint(new Point(-10, -10), this));

            cpCenter = new ControlPoint(this);
            cpHAngle = new ControlPoint(this);
            cpSideTop = new ControlPoint(this);
            cpSideBot = new ControlPoint(this);
            
            cpCenter.PositionChanged += OnCPPositionChanged;
            cpHAngle.PositionChanged += OnCPPositionChanged;
            cpSideTop.PositionChanged += OnCPPositionChanged;
            cpSideBot.PositionChanged += OnCPPositionChanged;

            PlaceControlPoints();
        }

        public override void CleanUp()
        {
            cpCenter.PositionChanged -= OnCPPositionChanged;
            cpHAngle.PositionChanged -= OnCPPositionChanged;
            cpSideTop.PositionChanged -= OnCPPositionChanged;
            cpSideBot.PositionChanged -= OnCPPositionChanged;
            base.CleanUp();
        }

        protected void PlaceControlPoints()
        {
            double ccos = Math.Cos(m_Alpha);
            double csin = Math.Sin(m_Alpha);
            double h = H / 2;
            Point[] pts = new Point[]{ new Point(h, LTop/2), new Point(h, -LTop/2), new Point(-h, -LBot/2), new Point(-h, LBot/2) };
            for(int i = 0; i < 4; i++)
                cps[i].Position = new Point(pts[i].X * ccos - pts[i].Y * csin + R, pts[i].X * csin + pts[i].Y * ccos + X );

            try
            {
                bProgrammaticChange = true;
                cpSideTop.Position = new Point(cps[1].Position.X, cps[1].Position.Y);
                cpSideBot.Position = new Point(cps[2].Position.X, cps[2].Position.Y);
                cpHAngle.Position = new Point(h * ccos + R, h * csin + X);
            }
            finally
            {
                bProgrammaticChange = false;
            }
        }

        public double H
        {
            get
            {
                return m_H;
            }
            set
            {
                m_H = value;
                PlaceControlPoints();
                OnZoneParametersChanged();
            }
        }

        public double Alpha
        {
            get
            {
                return m_Alpha;
            }
            set
            {
                m_Alpha = value;
                PlaceControlPoints();
                OnZoneParametersChanged();
            }
        }

        public double LTop
        {
            get
            {
                return m_LTop;
            }
            set
            {
                m_LTop = value;
                PlaceControlPoints();
                OnZoneParametersChanged();
            }
        }

        public double LBot
        {
            get
            {
                return m_LBot;
            }
            set
            {
                m_LBot = value;
                PlaceControlPoints();
                OnZoneParametersChanged();
            }
        }

        public double R
        {
            get
            {
                return cpCenter.Position.X;
            }
            set
            {
                cpCenter.Position = new Point(value,cpCenter.Position.Y);
                PlaceControlPoints();
                OnZoneParametersChanged();
            }
        }

        public double X
        {
            get
            {
                return cpCenter.Position.Y;
            }
            set
            {
                cpCenter.Position = new Point(cpCenter.Position.X, value);
                PlaceControlPoints();
                OnZoneParametersChanged();
            }
        }


        public override ZoneType Type
        {
            get { return ZoneType.ZoneTypeNgonTrapezoid; }
        }

        public override Geometry GetGeometry(Rect worldVisibleRect)
        {
            ngonFigure.Segments.Clear();
            if (cps.Count > 0)
            {
                ngonFigure.StartPoint = cps[0].Position;
                for (int i = 1; i < cps.Count; i++)
                    ngonFigure.Segments.Add(new LineSegment(cps[i].Position, true));
                ngonFigure.Segments.Add(new LineSegment(cps[0].Position, true));
            }
            return ngon;
        }

        protected void OnCPPositionChanged(object sender, ControlPointEventArgs e)
        {
            if (bProgrammaticChange) return;
            OnZoneParametersChanged();

            if (sender == cpCenter)
            {
                PlaceControlPoints();
                OnZoneParametersChanged();
            }
            else if(sender == cpHAngle)
            {
                double dx = cpHAngle.Position.X - R;
                double dy = cpHAngle.Position.Y - X;
                m_Alpha = Math.Atan2(dy, dx);
                m_H = 2 * Math.Sqrt(dy * dy + dx * dx);
                PlaceControlPoints();
                OnZoneParametersChanged();
            }
            else if(sender == cpSideTop)
            {
                double dx = e.NewVal.X - cpHAngle.Position.X;
                double dy = e.NewVal.Y - cpHAngle.Position.Y;
                m_LTop = 2 * Math.Sqrt(dx * dx + dy * dy);
                PlaceControlPoints();
                e.UseForceVal = true;
                e.ForceVal = cpSideTop.Position;
                OnZoneParametersChanged();
            }
            else if (sender == cpSideBot)
            {
                double dx = e.NewVal.X - (2 * cpCenter.Position.X - cpHAngle.Position.X);
                double dy = e.NewVal.Y - (2 * cpCenter.Position.Y - cpHAngle.Position.Y);
                m_LBot = 2 * Math.Sqrt(dx * dx + dy * dy);
                PlaceControlPoints();
                e.UseForceVal = true;
                e.ForceVal = cpSideBot.Position;
                OnZoneParametersChanged();
            }
        }

        public override ControlPoint[] ControlPoints
        {
            get
            {
                return new ControlPoint[] { cpCenter, cpHAngle, cpSideTop, cpSideBot };
            }
        }
    }


    public class ZoneCombination
    {
        private bool zone1;
        private bool zone2;
        private bool zone3;
        private bool zone4;

        public event EventHandler CombinationChanged;

        protected void OnCombinationChanged()
        {
            if(CombinationChanged != null)
                CombinationChanged(this, new EventArgs());
        }

        public ZoneCombination(bool Zone1Direct, bool Zone2Direct, bool Zone3Direct, bool Zone4Direct)
        {
            zone1 = Zone1Direct;
            zone2 = Zone2Direct;
            zone3 = Zone3Direct;
            zone4 = Zone4Direct;
        }

        internal bool GetZone(int Index)
        {
            switch (Index)
            {
            case 0:
                return zone1;
            case 1:
                return zone2;
            case 2:
                return zone3;
            case 3:
                return zone4;
            default:
                throw(new Exception("ZoneCombination::GetZone - wrong zone index"));
            }
        }

        public bool Zone1
        {
            get { return zone1; }
            set
            {
                if (zone1 != value)
                {
                    zone1 = value;
                    OnCombinationChanged();
                }
            }
        }

        public bool Zone2
        {
            get { return zone2; }
            set
            {
                if (zone2 != value)
                {
                    zone2 = value;
                    OnCombinationChanged();
                }
            }
        }

        public bool Zone3
        {
            get { return zone3; }
            set
            {
                if (zone3 != value)
                {
                    zone3 = value;
                    OnCombinationChanged();
                }
            }
        }

        public bool Zone4
        {
            get { return zone4; }
            set
            {
                if (zone4 != value)
                {
                    zone4 = value;
                    OnCombinationChanged();
                }
            }
        }
    }

    public class ZoneCombinations : List<ZoneCombination>
    {
        public event EventHandler ZoneCombinationChanged;

        public new void Add(ZoneCombination item)
        {
            base.Add(item);
            item.CombinationChanged += OnZoneCombinationChanged;
            OnZoneCombinationChanged(this, new EventArgs());
        }

        public new void Clear()
        {
            int nCount = Count;
            base.Clear();
            if (nCount > 0)
                OnZoneCombinationChanged(this, new EventArgs());
        }

        protected void OnZoneCombinationChanged(object sender, EventArgs e)
        {
            if (ZoneCombinationChanged != null)
                ZoneCombinationChanged(sender, e);
        }
    }
}




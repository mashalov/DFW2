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
        ZoneTypeHalfPlane,
        ZoneTypeNgon
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
        public Point NewVal { get; set; }
        public Point OldVal { get; set; }

        public ControlPointEventArgs(Point newVal, Point oldVal)
        {
            NewVal = newVal;
            OldVal = oldVal;
            Cancel = false;
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
   }
    
    internal static class StaticColors
    {
        public static SolidColorBrush CPBrush;
        public static SolidColorBrush CPOutline;
        public static SolidColorBrush ComplexZoneBrush1;
        public static SolidColorBrush ComplexZoneBrush2;
        public static SolidColorBrush Zone1Brush;
        public static SolidColorBrush Zone2Brush;
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
        }
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
                        if(!args.Cancel)
                            worldPosition = value;
                    }
                }
        }

        public static double ScreenRadius
        {
            get { return 6; }
        }
    }

    internal class ZoneHalfPlane : ZoneGeometry
    {
        private PathGeometry halfPlane = new PathGeometry();
        private ControlPoint cp1;
        private ControlPoint cp2;
        
        private PathFigure halfPlaneFigure = new PathFigure();

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

        public ZoneHalfPlane() : base()
        {
            halfPlane.Figures.Add(halfPlaneFigure);
            cp1 = new ControlPoint(this);
            cp2 = new ControlPoint(this);
            cp1.PositionChanged += OnCPPositionChanged;
            cp2.PositionChanged += OnCPPositionChanged;
        }

        public override Geometry GetGeometry(Rect worldVisibleRect)
        {
            halfPlaneFigure.Segments.Clear();

            Point InterPoint = new Point();
            List<DirPoint> checkList = new List<DirPoint>();
            if (IntersectLineWithCut(cp1.Position, cp2.Position, worldVisibleRect.TopLeft, worldVisibleRect.TopRight, ref InterPoint))
                checkList.Add(new ZoneHalfPlane.DirPoint(InterPoint,0));
            if(IntersectLineWithCut(cp1.Position, cp2.Position, worldVisibleRect.TopRight, worldVisibleRect.BottomRight, ref InterPoint))
                checkList.Add(new ZoneHalfPlane.DirPoint(InterPoint, 1));
            if(IntersectLineWithCut(cp1.Position, cp2.Position, worldVisibleRect.BottomRight, worldVisibleRect.BottomLeft, ref InterPoint))
                checkList.Add(new ZoneHalfPlane.DirPoint(InterPoint, 2));
            if(IntersectLineWithCut(cp1.Position, cp2.Position, worldVisibleRect.BottomLeft, worldVisibleRect.TopLeft, ref InterPoint))
                checkList.Add(new ZoneHalfPlane.DirPoint(InterPoint, 3));

            Point[] baseDirs = { worldVisibleRect.TopRight, worldVisibleRect.BottomRight, worldVisibleRect.BottomLeft, worldVisibleRect.TopLeft };
            if (checkList.Count == 2)
            {
                double Angle  = Math.Atan2(cp2.Position.Y - cp1.Position.Y, cp2.Position.X - cp1.Position.X) / Math.PI * 180;
                double Angle2 = Math.Atan2(checkList[1].pt. Y - checkList[0].pt.Y, checkList[1].pt. X - checkList[0].pt.X) / Math.PI * 180;

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
                    if(side == checkList[1].dir)
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
                if ((cp1.Position.Y - cp2.Position.Y) * (screenCenter.X - cp2.Position.X) +
                    (cp2.Position.X - cp1.Position.X) * (screenCenter.Y - cp2.Position.Y) <= 0 )
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

        private bool IntersectLineWithCut(Point Line1, Point Line2, Point Cut1, Point Cut2, ref Point Intersection)
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
                    if(div != 0)
                        t = (Line1.Y - Cut1.Y - dyldxl * (Line1.X - Cut1.X)) / div;
                }
                else
                {
                    if(dxcut != 0)
                        t = (Line1.X - Cut1.X) / dxcut;
                }

                if(t >= 0 && t <= 1)
                {
                    bRes = true;
                    Intersection.X = Cut1.X + dxcut * t;
                    Intersection.Y = Cut1.Y + dycut * t;
                }
            }
            return bRes;
        }

        protected void OnCPPositionChanged(object sender, EventArgs e)
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

        public override ZoneType Type
        {
            get { return ZoneType.ZoneTypeHalfPlane; }
        }

        public double R0
        {
            get { return cp1.Position.X; }
            set
            {
                if (Math.Abs(value - R0) > epsilon)
                {
                    cp1.Position = new Point(value, cp1.Position.Y);
                    PlaceControlPoints();
                    OnZoneParametersChanged();
                }
            }
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

        public double X0
        {
            get { return cp1.Position.Y; }
            set
            {
                if (Math.Abs(value - X0) > epsilon)
                {
                    cp1.Position = new Point(cp1.Position.X, value);
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
    }

    internal class ZoneNgon : ZoneGeometry
    {
        private PathGeometry ngon = new PathGeometry();
        private PathFigure ngonFigure = new PathFigure();
        private List<ControlPoint> cps = new List<ControlPoint>();

        public ZoneNgon() : base ()
        {
            cps.Add(new ControlPoint(new Point(2,2),this));
            cps.Add(new ControlPoint(new Point(2, 4),this));
            cps.Add(new ControlPoint(new Point(4, 4),this));
            cps.Add(new ControlPoint(new Point(4, 2),this));

            ngon.Figures.Add(ngonFigure);

            foreach (ControlPoint cp in cps)
                cp.PositionChanged += OnCPPositionChanged;
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
    
        public override Geometry GetGeometry(Rect worldVisibleRect)
        {
            ngonFigure.Segments.Clear();
            if(cps.Count > 0)
            {
                ngonFigure.StartPoint = cps[0].Position;
                for (int i = 1; i < cps.Count; i++)
                    ngonFigure.Segments.Add(new LineSegment(cps[i].Position, true));
                ngonFigure.Segments.Add(new LineSegment(cps[0].Position, true));
            }
            return ngon;
        }

        private bool CheckNgon()
        {
            int nVert = cps.Count;
            bool bIntersect = false;
            for (int i = 0; i < nVert - 2 && !bIntersect ; i++)
            {
                double dx1 = cps[i + 1].Position.X - cps[i].Position.X;
                double dy1 = cps[i + 1].Position.Y - cps[i].Position.Y;

                for (int j =  i + 1 ; j < nVert; j++)
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
                        double t2 = (dx1*(cps[i].Position.Y - y2) + dy1*(x2 - cps[i].Position.X)) / t;

                        double t1 = 0;
                        if (Math.Abs(dx1) > Math.Abs(dy1))
                            t1 = (x2 - cps[i].Position.X + dx2 * t2) / dx1;
                        else
                            t1 = (y2 - cps[i].Position.Y + dy2 * t2) / dy1;

                        if ((i == j - 1) && Math.Abs(t1-1) < 1E-3 && Math.Abs(t2) < 1E-3)
                            continue;

                        if ((i == 0) && (j == nVert - 1) && Math.Abs(t1) < 1E-3 && Math.Abs(t2 - 1) < 1E-3)
                            continue;

                        if ((t2 >= 0 && t2 <= 1) && (t1>=0 && t1<=1)) 
                        {
                            bIntersect = true;
                            break;
                        }

                    }
                }
            }
            return !bIntersect;
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

                    Point HitPt = new Point(dx,dy);

                    dx -= ScreenPoint.X;
                    dy -= ScreenPoint.Y;

                    if(dx*dx+dy*dy < 3)
                        hr = new ZoneHitTestResults(this,i,HitPt);
                }
                
            }
            return hr;
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




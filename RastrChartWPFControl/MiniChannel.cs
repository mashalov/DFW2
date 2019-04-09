using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;


namespace RastrChartWPFControl
{

    public class RangeChangedEventArgs : EventArgs
    {
        [System.Diagnostics.DebuggerBrowsable(System.Diagnostics.DebuggerBrowsableState.Never)]
        private double rangeMin;
        [System.Diagnostics.DebuggerBrowsable(System.Diagnostics.DebuggerBrowsableState.Never)]
        private double rangeMax;

        public RangeChangedEventArgs(double MinRange, double MaxRange) : base()
        {
            rangeMin = MinRange;
            rangeMax = MaxRange;
        }

        public double RangeMin
        {
            get { return rangeMin; }
            set { rangeMin = value; }
        }

        public double RangeMax
        {
            get { return rangeMax; }
            set { rangeMax = value; }
        }
    }

    public delegate void RangeChangedEventHandler(object sender, RangeChangedEventArgs e);

    public struct MiniPoint
    {
        public double X;
        public double Y;
        public double T;
        public double Angle;
        public double NormalX;
        public double NormalY;

        public MiniPoint(double x, double y)
        {
            this.X = x; 
            this.Y = y;
            this.T = 0.0;
            this.Angle = 0.0;
            this.NormalX = 0.0;
            this.NormalY = 0.0;
        }

        public MiniPoint(double x, double y, double t, double angle)
        {
            this.X = x;
            this.Y = y;
            this.Angle = angle;
            this.T = t;
            this.NormalX = 0.0;
            this.NormalY = 0.0;
        }

        public MiniPoint(double x, double y, double t)
        {
            this.X = x;
            this.Y = y;
            this.Angle = 0;
            this.T = t;
            this.NormalX = 0.0;
            this.NormalY = 0.0;
        }

        public static implicit operator Point(MiniPoint pt) 
        {
            return new Point(pt.X,pt.Y); 
        }

    }

    public class MiniPoints : System.Collections.IEnumerable
    {
        private MiniPoint[] points = new MiniPoint[0];
        public event EventHandler PointsChanged;

        private MiniPoint[] TransformedPoints;
        private MiniPoint[] RangedPoints;

        Point OldZoomFactor = new Point(-1,-1);
        Point range;

        public MiniPoints()
        {
            range.X = Double.NegativeInfinity;
            range.Y = Double.PositiveInfinity;
        }

        internal MiniPoint[] PointsRanged
        {
            get { return RangedPoints; }
        }

        internal Point GetRange()
        {
            return range;
        }

        internal void SetRange(Point Range, bool HodographMode)
        {
            if(range != Range)
            {
                range = Range;
                RangePoints(HodographMode);
                OnPointsChanged();
            }
        }

        private IntPair GetHodographRange(double Tmin, double Tmax)
        {
            IntPair ret = new IntPair(0, -1);
            if (points != null && points.Length > 0)
            {
                if (points[0].T >= Tmin)
                    ret.x0 = 0;
                else
                {
                    int beg = 0;
                    int end = points.Length - 1;

                    while (end - beg > 1)
                    {
                        int mid = beg + (end - beg) / 2;
                        if (points[mid].T > Tmin)
                            end = mid;
                        else
                            beg = mid;
                    }
                    ret.x0 = beg;
                }

                if (points[points.Length - 1].T < Tmax)
                    ret.x1 = points.Length - 1;
                else
                {
                    int beg = 0;
                    int end = points.Length - 1;

                    while (end - beg > 1)
                    {
                        int mid = beg + (end - beg) / 2;
                        if (points[mid].T < Tmax)
                            beg = mid;
                        else
                            end = mid;
                    }
                    ret.x1 = end;
                }
            }
            return ret;
        }

        private IntPair GetRange(double Xmin, double Xmax, bool HodographMode)
        {
            if (HodographMode)
                return GetHodographRange(Xmin, Xmax);

            IntPair ret = new IntPair(0, -1);
            if (points != null && points.Length > 0)
            {
                if (points[0].X >= Xmin)
                    ret.x0 = 0;
                else
                {
                    int beg = 0;
                    int end = points.Length - 1;

                    while (end - beg > 1)
                    {
                        int mid = beg + (end - beg) / 2;
                        if (points[mid].X > Xmin)
                            end = mid;
                        else
                            beg = mid;
                    }
                    ret.x0 = beg;
                }

                if (points[points.Length - 1].X < Xmax)
                    ret.x1 = points.Length - 1;
                else
                {
                    int beg = 0;
                    int end = points.Length - 1;

                    while (end - beg > 1)
                    {
                        int mid = beg + (end - beg) / 2;
                        if (points[mid].X < Xmax)
                            beg = mid;
                        else
                            end = mid;
                    }
                    ret.x1 = end;
                }
            }
            return ret;
        }

        protected void OnPointsChanged()
        {
            if (PointsChanged != null)
                PointsChanged(this, new EventArgs());
        }


        public bool SetPoints(MiniPoint[] Points)
        {
            bool bRes = true;
            points = Points;
            CalculateNormals();
            RangePoints(false);
            OnPointsChanged();
            return bRes;
        }

        protected void CalculateNormals()
        {
            if (points.Length == 0) return;

            int LastNormal = 0;

            double Angle1 = 0;
            double Angle2 = 0;
            double c, s;

            for ( int i = 0 ; i < points.Length - 1; i++)
            {
                double dx = points[i].X - points[LastNormal].X;
                double dy = points[i].Y - points[LastNormal].Y;
                if (Math.Abs(dx) > 1E-3 && Math.Abs(dy) > 1E-3)
                {
                    Angle2 = Math.Atan2(dy, dx);
                    double Angle = Angle1 + (Angle2 - Angle1) / 2 + Math.PI / 2;

                    c = Math.Cos(Angle);
                    s = Math.Sin(Angle);
                    for (int j = LastNormal; j < i; j++)
                    {
                        points[j].NormalX = c;  points[j].NormalY = s;
                    }
                    Angle1 = Angle2;
                    LastNormal = i;
                }
            }

            c = Math.Cos(Angle1);
            s = Math.Sin(Angle1);
            for (int j = LastNormal; j < points.Length; j++)
            {
                points[j].NormalX = c;  points[j].NormalY = s;
            }
        }

        protected void RangePoints(bool HodographMode)
        {
            IntPair begend = GetRange(range.X, range.Y,HodographMode);
            if (begend.x0 < begend.x1)
            {
                RangedPoints = new MiniPoint[begend.x1 - begend.x0 + 1];
                for (int i = begend.x0; i <= begend.x1; i++)
                {
                    RangedPoints[i - begend.x0] = points[i];
                    /*RangedPoints[i - begend.x0].X = points[i].X;
                    RangedPoints[i - begend.x0].Y = points[i].Y;
                    RangedPoints[i - begend.x0].T = points[i].T;*/
                }
                OldZoomFactor.X = -1;
            }
        }

        public MiniPoint[] TransformPoints(Point Scale)
        {
            if ( Scale != OldZoomFactor && points.Length > 0 )
            {
                OldZoomFactor = Scale;
                MiniPoint[] tempPoint = new MiniPoint[RangedPoints.Length];
                int PrevAdded = 0;
                int nCount = 1;
                tempPoint[PrevAdded].X = RangedPoints[0].X;
                tempPoint[PrevAdded].Y = -RangedPoints[0].Y;
                for (int i = 1; i < RangedPoints.Length; i++)
                {
                    double dx = RangedPoints[PrevAdded].X;
                    double dy = RangedPoints[PrevAdded].Y;
                    dx -= RangedPoints[i].X;
                    dy -= RangedPoints[i].Y;
                    dx *= Scale.X;
                    dy *= Scale.Y;
                    dx *= dx;
                    dy *= dy;
                    if (dx + dy > 1.5)
                    {
                        tempPoint[nCount].X = RangedPoints[i].X;
                        tempPoint[nCount++].Y = -RangedPoints[i].Y;
                        PrevAdded = i;
                    }
                }
                TransformedPoints = new MiniPoint[nCount];
                for (int i = 0; i < nCount; i++)
                    TransformedPoints[i] = tempPoint[i];

            }
            return TransformedPoints;
        }

        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
        {
            return points.GetEnumerator();
        }

        public int Count
        {
            get { return points.Length; }
        }
    }

    public class MiniChannel
    {
        private MiniPoints points = new MiniPoints();
        private MiniChannels channels;
        private string tag;
        private Rect Bound;
        private StreamGeometry LineStream;
        private Path LinePath;
        Point availableRange;
        Color channelColor = Colors.Black;
        private bool hodographMode;

        public event EventHandler ChannelChanged;
        public event RangeChangedEventHandler RangeChanged;
        
        protected void OnChannelChanged()
        {
            if (ChannelChanged != null)
                ChannelChanged(this, new EventArgs());
        }

        protected void OnRangeChanged()
        {
            if (RangeChanged != null)
                RangeChanged(this, new RangeChangedEventArgs(availableRange.X,availableRange.Y));
        }

        public Rect GetBounds()
        {
            return Bound;
        }

        public Point Range
        {
            get { return points.GetRange(); }
            set
            {
                if(points.GetRange() != value)
                {
                    points.SetRange(value,hodographMode);
                    CalculateBounds();
                }
            }
        }

        public Color Color
        {
            get { return channelColor; }
            set
            {
                if (channelColor != value)
                {
                    channelColor = value;
                    OnChannelChanged();
                }
            }
        }

        public Point AvailableRange
        {
            get { return availableRange; }
        }

        protected void CalculateAvailableRange()
        {
            availableRange = new Point(Double.PositiveInfinity, Double.NegativeInfinity);


            foreach (MiniPoint pt in points)
            {
                double Value = pt.X;
                if (hodographMode)
                    Value = pt.T;

                if (Value < availableRange.X)
                    availableRange.X = Value;
                if (Value > availableRange.Y)
                    availableRange.Y = Value;
            }
            if (Double.IsInfinity(availableRange.X) || Double.IsInfinity(availableRange.Y))
                availableRange = new Point(-0.5, 0.5);
        }

        internal bool NoData()
        {
            return points.Count == 0;
        }

        internal void CalculateBounds()
        {
            Bound = new Rect(-0.5,-0.5,1.0,1.0);
            double Left   = Double.PositiveInfinity;
            double Right  = Double.NegativeInfinity;
            double Top    = Double.PositiveInfinity;
            double Bottom = Double.NegativeInfinity;

            if (!hodographMode)
            {
                Left = Math.Max(AvailableRange.X, Range.X);
                Right = Math.Min(AvailableRange.Y, Range.Y);
            }

            if (points.PointsRanged != null)
            {
                foreach (Point pt in points.PointsRanged)
                {
                    if (-pt.Y < Top)
                        Top = -pt.Y;
                    if (-pt.Y > Bottom)
                        Bottom = -pt.Y;
                }

                if (hodographMode)
                {
                    foreach (Point pt in points.PointsRanged)
                    {
                        if (pt.X < Left)
                            Left = pt.X;
                        if (pt.X > Right)
                            Right = pt.X;
                    }
                }

                if (!Double.IsInfinity(Left) && !Double.IsInfinity(Top) &&
                    !Double.IsInfinity(Right) && !Double.IsInfinity(Bottom))
                {
                    Bound.X = Left;
                    Bound.Y = Top;
                    Bound.Width = Math.Abs(Right - Left);
                    Bound.Height = Math.Abs(Bottom - Top);

                    if (Bound.Width > 0 && Bound.Height > 0)
                    {
                        Bound.Y -= Bound.Height * 0.05 / 2.0;
                        Bound.Height *= 1.05;
                    }
                    else
                    {
                        if (Bound.Width == 0)
                        {
                            Bound.Width = 1.0;
                            Bound.X -= 0.5;
                        }
                        if (Bound.Height == 0)
                        {
                            Bound.Height = 1.0;
                            Bound.Y -= 0.5;
                        }
                    }
                }
            }
            else
            {
                if (Double.IsInfinity(Left))
                    Left = -0.5;
                if (Double.IsInfinity(Right))
                    Right = 0.5;

                Bound.X = Left;
                Bound.Width = Right - Left;
            }
        }

        public MiniChannel(MiniChannels Channels)
        {
            channels = Channels;
            points.PointsChanged += OnPointsChanged;
            LineStream = new StreamGeometry();
            LinePath = new Path();
            LinePath.StrokeLineJoin = PenLineJoin.Bevel;
            hodographMode = false;
        }

        public MiniPoints Points
        {
            get { return points; }
        }

        public string Legend
        {
            get;
            set;
        }

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
                    if (channels.GetChannelByTag(value) == null)
                        tag = value;
                    else
                        throw new Exception(String.Format("Channel tag \"{0}\" is already in use", value));
                }
            }
        }

        internal bool HodographMode
        {
            get { return hodographMode; }
            set { hodographMode = value; }
        }

        protected void OnPointsChanged(object sender, EventArgs e)
        {
            Point OldRange = availableRange;
            CalculateAvailableRange();
            if (OldRange != availableRange)
                OnRangeChanged();
            CalculateBounds();
            OnChannelChanged();
        }

        public Path GetGodographPath(Point ZoomFactor, Point Offset)
        {
            LineStream.Clear();
            MiniPoint[] TransformedPoints = points.TransformPoints(ZoomFactor);
            if (TransformedPoints != null)
            {
                using (StreamGeometryContext ctx = LineStream.Open())
                {
                    ctx.BeginFigure(TransformedPoints[0], false, false);
                    for (int i = 0; i < TransformedPoints.Length; i++)
                        ctx.LineTo(TransformedPoints[i], true, false);
                }

                LineStream.FillRule = FillRule.EvenOdd;
                TransformGroup tg = new TransformGroup();
                ScaleTransform st = new ScaleTransform(ZoomFactor.X, ZoomFactor.Y);
                TranslateTransform tt = new TranslateTransform(Offset.X, Offset.Y);
                tg.Children.Add(st);
                tg.Children.Add(tt);
                LineStream.Transform = tg;
                LinePath.Data = LineStream;
                LinePath.Stroke = new SolidColorBrush(channelColor);
                LinePath.StrokeThickness = 3;
                LinePath.StrokeLineJoin = PenLineJoin.Round;
            }
            return LinePath;
        }

        public bool FindTimeCoordinates(double Time, ref Point point)
        {
            MiniPoint pt = new MiniPoint(0, 0, Time);
            bool bRes = GetClosestTimePoint(Time, ref pt);
            if (bRes)
            {
                point.X = pt.X;
                point.Y = pt.Y;
            }
            return bRes;
        }

        internal bool GetClosestTimePoint(double Time, ref MiniPoint ResultPoint)
        {
            bool bRes = false;
            if (points != null && points.PointsRanged != null && points.PointsRanged.Length > 0)
            {
                int beg = 0;
                int end = points.PointsRanged.Length - 1;
                if (points.PointsRanged[beg].T > Time)
                    return false;

                if (points.PointsRanged[end].T < Time)
                    return false;

                bRes = true;
                while (end - beg > 1)
                {
                    int mid = beg + (end - beg) / 2;
                    ResultPoint = points.PointsRanged[mid];
                    double diff = Time - ResultPoint.T;
                    if (diff > 0)
                        beg = mid;
                    else
                        if (diff < 0)
                            end = mid;
                        else
                            break;
                }

                if (Math.Abs(points.PointsRanged[beg].T - Time) < Math.Abs(points.PointsRanged[end].T - Time))
                    ResultPoint = points.PointsRanged[beg];
                else
                    ResultPoint = points.PointsRanged[end];
            }
            return bRes;
        }
      
        public Path GetChannelPath(Point ZoomFactor, Point Offset)
        {
            LineStream.Clear();
            MiniPoint[] TransformedPoints = points.TransformPoints(ZoomFactor);
            if (TransformedPoints != null)
            {
                using (StreamGeometryContext ctx = LineStream.Open())
                {
                    ctx.BeginFigure(TransformedPoints[0], false, false);
                    for (int i = 0; i < TransformedPoints.Length; i++)
                        ctx.LineTo(TransformedPoints[i], true, false);
                }

                LineStream.FillRule = FillRule.EvenOdd;
                TransformGroup tg = new TransformGroup();
                ScaleTransform st = new ScaleTransform(ZoomFactor.X, ZoomFactor.Y);
                TranslateTransform tt = new TranslateTransform(Offset.X, Offset.Y);
                tg.Children.Add(st);
                tg.Children.Add(tt);
                LineStream.Transform = tg;
                LinePath.Data = LineStream;
                LinePath.Stroke = new SolidColorBrush(channelColor);
                LinePath.StrokeThickness = 3;
                LinePath.StrokeLineJoin = PenLineJoin.Round;
            }
            return LinePath;
        }
    }

    public class MiniChannels : System.Collections.IEnumerable
    {
        private List<MiniChannel> Channels = new List<MiniChannel>();
        public event EventHandler Update;

        public event RangeChangedEventHandler RangeChanged;

        private Rect Bounds = new Rect(-0.5,-0.5,1.0,1.0);
        private Point range;
        private Point availableRange = new Point(-0.5, 0.5);
        private bool hodographMode;

        private void OnUpdate()
        {
            if (Update != null)
                Update(this, new EventArgs());
        }

        protected void OnRangeChanged()
        {
            if (RangeChanged != null)
                RangeChanged(this, new RangeChangedEventArgs(availableRange.X, availableRange.Y));
        }

        public Point Range
        {
            get { return range; }
            set
            {
                if (range != value)
                {
                    range = value;
                    foreach (MiniChannel mc in Channels)
                        mc.Range = range;
                    CalculateBounds();
                    OnUpdate();
                }
            }
        }

        public MiniChannel Add(string Legend)
        {
            MiniChannel newChannel = new MiniChannel(this);
            newChannel.Legend = Legend;
            newChannel.HodographMode = hodographMode;

            long Hash = Convert.ToInt64(newChannel.GetHashCode().ToString());
            while (true)
            {
                try
                {
                    newChannel.Tag = Hash.ToString();
                    break;
                }
                catch (Exception)
                {
                    Hash++;
                }
            }
            Channels.Add(newChannel);
            newChannel.ChannelChanged += OnChannelChanged;
            OnUpdate();
            return newChannel;
        }

        public MiniChannel GetChannelByTag(string Tag)
        {
            foreach (MiniChannel mc in Channels)
            {
                if (mc.Tag == Tag) return mc;
            }
            return null;
        }

        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
        {
            return Channels.GetEnumerator();
        }

        public Rect GetBounds()
        {
            return Bounds;
        }

        public Point AvailableRange
        {
            get { return availableRange; }
        }

        protected void CalculateAvailableRange()
        {
            bool bFirst = true;
            foreach (MiniChannel mc in Channels)
            {
                if (bFirst)
                {
                    bFirst = false;
                    availableRange = mc.AvailableRange;
                }
                else
                {
                    Point onceRange = mc.AvailableRange;
                    if (onceRange.X < availableRange.X) availableRange.X = onceRange.X;
                    if (onceRange.Y < availableRange.Y) availableRange.Y = onceRange.Y;
                }
            }
        }

        protected void CalculateBounds()
        {
            if (Channels.Count > 0)
            {
                bool bFirst = true;
                foreach (MiniChannel mc in Channels)
                {
                    mc.CalculateBounds();
                    if (bFirst)
                    {
                        bFirst = false;
                        Bounds = mc.GetBounds();
                    }
                    else
                        Bounds.Union(mc.GetBounds());
                }
            }
            else
            {
                Bounds.X = Math.Max(AvailableRange.X, Range.X);
                Bounds.Width = Math.Min(AvailableRange.Y, Range.Y) - Bounds.X;
            }
        }

        protected void OnChannelChanged(object sender, EventArgs e)
        {
            CalculateBounds();
            Point OldRange = availableRange;
            CalculateAvailableRange();
            if (OldRange != availableRange)
                OnRangeChanged();
            OnUpdate();
        }

        internal bool HodographMode
        {
            get { return hodographMode; }
            set 
            {
                if (hodographMode != value)
                {
                    hodographMode = value;
                    foreach (MiniChannel mc in Channels)
                        mc.HodographMode = value;
                }
                OnUpdate();
            }

        }

        internal bool NoData()
        {
            bool bRes = true;
            foreach (MiniChannel mc in Channels)
            {
                if (!mc.NoData())
                {
                    bRes = false;
                    break;
                }
            }
            return bRes;
        }

    }

    internal class GridHittest
    {
        private double width;
        private double height;
        private int widthBlocks;
        private int heightBlocks;
        private bool[] grid;

        internal GridHittest(double Width, double Height, int WidthBlocks, int HeightBlocks)
        {
            width = Width;
            height = Height;
            widthBlocks = WidthBlocks;
            heightBlocks = HeightBlocks;
            grid = new bool[widthBlocks*heightBlocks];
        }

        private int GetPosition(double X, double Y)
        {
            int BaseX = (int)Math.Floor(X / width * (widthBlocks-1));
            int BaseY = (int)Math.Floor(Y / height * (heightBlocks-1));
            if (BaseX < widthBlocks && BaseY < heightBlocks && BaseX >=0 && BaseY >=0)
            {
                return BaseX + BaseY * heightBlocks;
            }
            else
                return -1;
        }

        internal bool Check(double X, double Y)
        {
            int Position = GetPosition(X, Y);
            if (Position >= 0)
                return grid[Position];
            else
                return false;
        }

        internal void Set(double X, double Y)
        {
            int Position = GetPosition(X, Y);
            if (Position >= 0)
                grid[Position] = true;
        }
    }
}


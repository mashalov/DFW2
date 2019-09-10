using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Media;
using System.Windows.Shapes;

namespace RastrChartWPFControl
{
    public class Zone
    {
        private ZoneGeometry zoneGeometry;

        public event EventHandler ZoneTypeChanged;
        public event EventHandler ZoneParametersChanged;

        internal ZoneGeometry ZoneGeometry
        {
            get { return zoneGeometry; }
        }

        public ZoneType Type
        {
            get
            {
                if (zoneGeometry == null)
                    return ZoneType.ZoneTypeOff;
                else
                    return zoneGeometry.Type;
            }

            set
            {
                if (Type != value)
                {
                    if (zoneGeometry != null)
                        zoneGeometry.ZoneParametersChanged -= OnZoneParametersChanged;

                    switch (value)
                    {
                        case ZoneType.ZoneTypeOff:
                            zoneGeometry = null;
                            break;
                        case ZoneType.ZoneTypeEllipse:
                            zoneGeometry = new ZoneEllipse();
                            break;
                        case ZoneType.ZoneTypeHalfPlane:
                            zoneGeometry = new ZoneHalfPlane();
                            break;
                        case ZoneType.ZoneTypeNgon:
                            zoneGeometry = new ZoneNgon();
                            break;
                        case ZoneType.ZoneTypeHalfPlaneSingle:
                            zoneGeometry = new ZoneHalfPlaneSingle();
                            break;
                        case ZoneType.ZoneTypeNgonTrapezoid:
                            zoneGeometry = new ZoneNgonTrapezoid();
                            break;
                        case ZoneType.ZoneTypeCircle:
                            zoneGeometry = new ZoneCircle();
                            break;
                        case ZoneType.ZoneTypeHalfPlaneCenter:
                            zoneGeometry = new ZoneHalfPlaneCenter();
                            break;
                    }
                    
                    if (zoneGeometry != null)
                        zoneGeometry.ZoneParametersChanged += OnZoneParametersChanged;

                    if (ZoneTypeChanged != null)
                        ZoneTypeChanged(this, new EventArgs());
                }
            }
        }

        public double Z
        {
            get { return ((ZoneEllipse)zoneGeometry).Z;  }
            set { ((ZoneEllipse)zoneGeometry).Z = value; }
        }

        public double K
        {
            get { return ((ZoneEllipse)zoneGeometry).K;  }
            set { ((ZoneEllipse)zoneGeometry).K = value; }
        }

        public double R0
        {
            get 
            { 
                if(zoneGeometry.GetType() == typeof(ZoneEllipse))
                    return ((ZoneEllipse)zoneGeometry).R0;  
                if(zoneGeometry.GetType() == typeof(ZoneHalfPlane) || zoneGeometry.GetType() == typeof(ZoneHalfPlaneSingle))
                    return ((ZoneHalfPlane)zoneGeometry).R0;
                if (zoneGeometry.GetType() == typeof(ZoneNgonTrapezoid))
                    return ((ZoneNgonTrapezoid)zoneGeometry).R;
                if (zoneGeometry.GetType() == typeof(ZoneCircle))
                    return ((ZoneCircle)zoneGeometry).Zr;
                throw new Exception();
            }
            set 
            {

                if (zoneGeometry.GetType() == typeof(ZoneEllipse))
                    ((ZoneEllipse)zoneGeometry).R0 = value;
                else if (zoneGeometry.GetType() == typeof(ZoneHalfPlane))
                    ((ZoneHalfPlane)zoneGeometry).R0 = value;
                else if (zoneGeometry.GetType() == typeof(ZoneNgonTrapezoid))
                    ((ZoneNgonTrapezoid)zoneGeometry).R = value;
                else if (zoneGeometry.GetType() == typeof(ZoneCircle))
                    ((ZoneCircle)zoneGeometry).Zr = value;
                else throw new Exception();
            }
        }

        public double X0
        {
            get 
            {
                if (zoneGeometry.GetType() == typeof(ZoneEllipse))
                    return ((ZoneEllipse)zoneGeometry).X0;
                if (zoneGeometry.GetType() == typeof(ZoneHalfPlane) || zoneGeometry.GetType() == typeof(ZoneHalfPlaneSingle))
                    return ((ZoneHalfPlane)zoneGeometry).X0;
                if (zoneGeometry.GetType() == typeof(ZoneNgonTrapezoid))
                    return ((ZoneNgonTrapezoid)zoneGeometry).X;
                if (zoneGeometry.GetType() == typeof(ZoneCircle))
                    return ((ZoneCircle)zoneGeometry).Zo;
                throw new Exception();
            }
            set 
            {
                if (zoneGeometry.GetType() == typeof(ZoneEllipse))
                    ((ZoneEllipse)zoneGeometry).X0 = value;
                else if (zoneGeometry.GetType() == typeof(ZoneHalfPlane))
                    ((ZoneHalfPlane)zoneGeometry).X0 = value;
                else if (zoneGeometry.GetType() == typeof(ZoneNgonTrapezoid))
                    ((ZoneNgonTrapezoid)zoneGeometry).X = value;
                else if (zoneGeometry.GetType() == typeof(ZoneCircle))
                    ((ZoneCircle)zoneGeometry).Zo = value;
                else throw new Exception();
            }
        }

        public double S
        {
            get
            {
                if (zoneGeometry.GetType() == typeof(ZoneNgonTrapezoid))
                    return ((ZoneNgonTrapezoid)zoneGeometry).H;
                else if(zoneGeometry.GetType() == typeof(ZoneHalfPlaneSingle))
                {
                    double r0 = ((ZoneHalfPlaneSingle)zoneGeometry).R0;
                    double x0 = ((ZoneHalfPlaneSingle)zoneGeometry).X0;
                    return Math.Sqrt(r0 * r0 + x0 * x0);
                }
                throw new Exception();
            }
            set
            {
                if (zoneGeometry.GetType() == typeof(ZoneNgonTrapezoid))
                    ((ZoneNgonTrapezoid)zoneGeometry).H = value;
                else if (zoneGeometry.GetType() == typeof(ZoneHalfPlaneSingle))
                {
                    double r0 = ((ZoneHalfPlaneSingle)zoneGeometry).R0;
                    double x0 = ((ZoneHalfPlaneSingle)zoneGeometry).X0;

                    double checkVal = Math.Abs(value) > 0.001 ? value : 0.001;

                    double Angle = Math.Atan2(x0, r0);
                    ((ZoneHalfPlaneSingle)zoneGeometry).R0 = checkVal * Math.Cos(Angle);
                    ((ZoneHalfPlaneSingle)zoneGeometry).X0 = checkVal * Math.Sin(Angle);
                }
                else
                    throw new Exception();
            }
        }
        
        public double R1
        {
            get { return ((ZoneHalfPlane)zoneGeometry).R1; }
            set { ((ZoneHalfPlane)zoneGeometry).R1 = value; }
        }

        public double X1
        {
            get { return ((ZoneHalfPlane)zoneGeometry).X1; }
            set { ((ZoneHalfPlane)zoneGeometry).X1 = value; }
        }

        public double Phi
        {
            get
            {
                if (zoneGeometry.GetType() == typeof(ZoneEllipse))
                    return ((ZoneEllipse)zoneGeometry).Phi;
                else if (zoneGeometry.GetType() == typeof(ZoneHalfPlaneSingle))
                {
                    double r0 = ((ZoneHalfPlaneSingle)zoneGeometry).R0;
                    double x0 = ((ZoneHalfPlaneSingle)zoneGeometry).X0;
                    return Math.Atan2(x0, r0) * 180.0 / Math.PI;
                }
                else if (zoneGeometry.GetType() == typeof(ZoneNgonTrapezoid))
                    return ((ZoneNgonTrapezoid)zoneGeometry).Alpha;
                else if(zoneGeometry.GetType() == typeof(ZoneHalfPlaneCenter))
                    return ((ZoneHalfPlaneCenter)zoneGeometry).Phi;
                else if (zoneGeometry.GetType() == typeof(ZoneCircle))
                    return ((ZoneCircle)zoneGeometry).Phi;
                throw new Exception();
            }
            set
            {
                if (zoneGeometry.GetType() == typeof(ZoneEllipse))
                    ((ZoneEllipse)zoneGeometry).Phi = value;
                else if (zoneGeometry.GetType() == typeof(ZoneHalfPlaneSingle))
                {
                    double r0 = ((ZoneHalfPlaneSingle)zoneGeometry).R0;
                    double x0 = ((ZoneHalfPlaneSingle)zoneGeometry).X0;
                    double s = Math.Sqrt(r0 * r0 + x0 * x0);
                    ((ZoneHalfPlaneSingle)zoneGeometry).R0 = s * Math.Cos(value * Math.PI / 180);
                    ((ZoneHalfPlaneSingle)zoneGeometry).X0 = s * Math.Sin(value * Math.PI / 180);
                }
                else if (zoneGeometry.GetType() == typeof(ZoneNgonTrapezoid))
                    ((ZoneNgonTrapezoid)zoneGeometry).Alpha = value;
                else if (zoneGeometry.GetType() == typeof(ZoneHalfPlaneCenter))
                    ((ZoneHalfPlaneCenter)zoneGeometry).Phi = value;
                else if (zoneGeometry.GetType() == typeof(ZoneCircle))
                    ((ZoneCircle)zoneGeometry).Phi = value;
                else
                    throw new Exception();
            }
        }

        public double LTop
        {
            get
            {
                if (zoneGeometry.GetType() == typeof(ZoneNgonTrapezoid))
                    return ((ZoneNgonTrapezoid)zoneGeometry).LTop;
                throw new Exception();
            }

            set
            {
                if (zoneGeometry.GetType() == typeof(ZoneNgonTrapezoid))
                    ((ZoneNgonTrapezoid)zoneGeometry).LTop = value;
                else
                    throw new Exception();
            }
        }

        public double LBot
        {
            get
            {
                if (zoneGeometry.GetType() == typeof(ZoneNgonTrapezoid))
                    return ((ZoneNgonTrapezoid)zoneGeometry).LBot;
                throw new Exception();
            }

            set
            {
                if (zoneGeometry.GetType() == typeof(ZoneNgonTrapezoid))
                    ((ZoneNgonTrapezoid)zoneGeometry).LBot = value;
                else
                    throw new Exception();
            }
        }


        public Point[] Points
        {
            get 
            { 
                Point[] points = new Point[zoneGeometry.ControlPoints.Length];
                int i = 0;
                foreach(ControlPoint cp in zoneGeometry.ControlPoints)
                    points[i++] = cp.Position;
                return points;
            }

            set
            {
                if (!((ZoneNgon)zoneGeometry).SetControlPoints(value))
                    throw new Exception("Попытка установить набор точек многоугольника, образующий самопересечение");
            }
        }

        protected void OnZoneParametersChanged(object sender, EventArgs e)
        {
            if (ZoneParametersChanged != null)
                ZoneParametersChanged(sender, e);
        }

        public bool PointInZone(Point WorldPoint)
        {
            return zoneGeometry == null ? false : zoneGeometry.PointInZone(WorldPoint);
        }
    }
       


    public class ComplexZoneRelay
    {
        protected Zone[] zones = new Zone[4];
        protected ZoneCombinations zoneCombinations = new ZoneCombinations();

        public event EventHandler ZoneTypeChanged;
        public event EventHandler ZoneCombinationChanged;
        public event EventHandler ZoneParametersChanged;

        public ComplexZoneRelay()
        {
            for (int i = 0; i < 4; i++)
            {
                zones[i] = new Zone();
                zones[i].ZoneTypeChanged += OnZoneTypeChanged;
                zones[i].ZoneParametersChanged += OnZoneParametersChanged;
            }
            zoneCombinations.ZoneCombinationChanged += OnZoneCombinationChanged;
        }

        public bool PointInZone(Point WorldPoint)
        {
            int [] ZoneHitResults = new int[4];
            bool bActiveZonePresent = false;
            bool bOneZoneHit = false;

            for (int i = 0; i < 4; i++)
            {
                if (zones[i].Type != ZoneType.ZoneTypeOff)
                {
                    ZoneHitResults[i] = zones[i].PointInZone(WorldPoint) ? 1 : 0;
                    bActiveZonePresent = true;
                }
                else
                    ZoneHitResults[i] = 2; // ignore zone;
            }

            bool bRes = false;

            if (bOneZoneHit)
                bRes = true;
            else
            {
                if (bActiveZonePresent)
                {
                    foreach(ZoneCombination zc in zoneCombinations)
                    {
                        bRes = true;
                        for (int i = 0; i < 4; i++)
                        {
                            int ZoneHit = ZoneHitResults[i];
                            if (ZoneHit == 2) continue;
                            bRes = bRes && ((ZoneHit > 0 ? true : false) == zc.GetZone(i));
                            if (!bRes)
                                break;
                        }
                        if (bRes)
                            break;
                    }
                }
            }
            return bRes;
        }

        public Zone this[int index]
        {
            get
            {
                if (index >= 0 && index < 4)
                    return zones[index];
                else
                    throw new Exception("ComplexZoneRelay:Zone get - wrong index");
            }
        }
        public ZoneCombinations ZoneCombinations
        {
            get
            {
                return zoneCombinations;
            }
        }

        protected void OnZoneParametersChanged(object sender, EventArgs e)
        {
            if (ZoneParametersChanged != null)
                ZoneParametersChanged(sender, e);
        }

        protected void OnZoneTypeChanged(object sender, EventArgs e)
        {
            if (ZoneTypeChanged != null)
                ZoneTypeChanged(sender, e);
        }

        protected void OnZoneCombinationChanged(object sender, EventArgs e)
        {
            if (ZoneCombinationChanged != null)
                ZoneCombinationChanged(sender, e);
        }
    }

    public class ZoneHalfPlaneSingleRelay
    {
        private Zone zone;
        public event EventHandler ZoneParametersChanged;
        public Line sVector = new Line();

        public bool ZoneOn
        {
            get;
            set;
        }
        public ZoneHalfPlaneSingleRelay()
        {
            zone = new Zone();
            zone.Type = ZoneType.ZoneTypeHalfPlaneSingle;
            zone.ZoneParametersChanged += OnZoneParametersChanged;
            ZoneOn = false;
            sVector.StrokeThickness = 1;
            sVector.Stroke = Brushes.Gray;
        }
        public Zone Zone
        {
            get
            {
                return zone;
            }
        }
        protected void OnZoneParametersChanged(object sender, EventArgs e)
        {
            if (ZoneParametersChanged != null)
                ZoneParametersChanged(sender, e);
        }
    }

    public class ZoneFZ : ComplexZoneRelay
    {
        private Zone zoneZ1, zoneZ2, zoneZ;
        public bool ZoneOn
        {
            get;
            set;
        }

        public ZoneFZ() : base()
        {
            zones[0].Type = ZoneType.ZoneTypeCircle;
            zones[1].Type = ZoneType.ZoneTypeCircle;
            zones[2].Type = ZoneType.ZoneTypeHalfPlaneCenter;
            zones[3].Type = ZoneType.ZoneTypeOff;

            zones[0].Phi = Math.PI / 2.0;
            zones[1].Phi = -Math.PI / 2.0;
            zones[0].R0 = 30;
            zones[0].X0 = -20;
            zones[1].R0 = 50;
            zones[1].X0 = -10;
            zoneCombinations.Add(new ZoneCombination(true, true, true, false));
        }
    }


    public class ZoneKPA
    {
        private Zone fineZone, coarseZone;
        public event EventHandler ZoneParametersChanged;
        public Line sVector = new Line();
        public bool ZoneOn
        {
            get;
            set;
        }

        public Zone this[int index]
        {
            get
            {
                switch (index)
                {
                    case 0:
                        return fineZone;
                    case 1:
                        return coarseZone;
                    default:
                        throw new Exception("ZoneKPA::Zone get - wrong index");
                }
            }
        }

        public ZoneKPA()
        {
            fineZone = new Zone();
            coarseZone = new Zone();
            fineZone.ZoneParametersChanged += OnZoneParametersChanged;
            coarseZone.ZoneParametersChanged += OnZoneParametersChanged;
            fineZone.Type = ZoneType.ZoneTypeNgonTrapezoid;
            coarseZone.Type = ZoneType.ZoneTypeNgonTrapezoid;
            sVector.StrokeThickness = 1.5;
            sVector.Stroke = new SolidColorBrush(StaticColors.Zone1Brush.Color);
            ZoneOn = false;
            fineZone.S = 20;
            fineZone.Phi = Math.PI / 2.0;
            fineZone.LTop = 30;
            fineZone.LBot = 20;
            coarseZone.LTop = 20;
            coarseZone.LBot = 10;
        }

        protected void OnZoneParametersChanged(object sender, EventArgs e)
        {
            ZoneNgonTrapezoid fineTrapezoid   = (ZoneNgonTrapezoid)fineZone.ZoneGeometry;
            ZoneNgonTrapezoid coarseTrapezoid = (ZoneNgonTrapezoid)coarseZone.ZoneGeometry;
            try
            {
                fineZone.ZoneParametersChanged -= OnZoneParametersChanged;
                coarseZone.ZoneParametersChanged -= OnZoneParametersChanged;

                if (sender == fineTrapezoid)
                {
                    coarseTrapezoid.R = fineTrapezoid.R;
                    coarseTrapezoid.Alpha = fineTrapezoid.Alpha;
                    coarseTrapezoid.X = fineTrapezoid.X;
                    coarseTrapezoid.H = fineTrapezoid.H;

                    if (fineTrapezoid.LTop < coarseTrapezoid.LTop)
                        coarseTrapezoid.LTop = fineTrapezoid.LTop;

                    if (fineTrapezoid.LBot < coarseTrapezoid.LBot)
                        coarseTrapezoid.LBot = fineTrapezoid.LBot;
                }
                else if(sender == coarseTrapezoid)
                {
                    fineTrapezoid.R = coarseTrapezoid.R;
                    fineTrapezoid.Alpha = coarseTrapezoid.Alpha;
                    fineTrapezoid.X = coarseTrapezoid.X;
                    fineTrapezoid.H = coarseTrapezoid.H;

                    if (fineTrapezoid.LTop < coarseTrapezoid.LTop)
                        fineTrapezoid.LTop = coarseTrapezoid.LTop;

                    if (fineTrapezoid.LBot < coarseTrapezoid.LBot)
                        fineTrapezoid.LBot = coarseTrapezoid.LBot;
                }
            }
            finally
            {
                fineZone.ZoneParametersChanged += OnZoneParametersChanged;
                coarseZone.ZoneParametersChanged += OnZoneParametersChanged;
            }

            if (ZoneParametersChanged != null)
                ZoneParametersChanged(sender, e);
        }
    }
}

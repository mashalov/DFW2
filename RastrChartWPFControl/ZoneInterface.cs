using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Media;

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
                if(zoneGeometry.GetType() == typeof(ZoneHalfPlane))
                    return ((ZoneHalfPlane)zoneGeometry).R0;
                throw new Exception();
            }
            set 
            {

                if (zoneGeometry.GetType() == typeof(ZoneEllipse))
                    ((ZoneEllipse)zoneGeometry).R0 = value;
                else if (zoneGeometry.GetType() == typeof(ZoneHalfPlane))
                    ((ZoneHalfPlane)zoneGeometry).R0 = value;
                else throw new Exception();
            }
        }

        public double X0
        {
            get 
            {
                if (zoneGeometry.GetType() == typeof(ZoneEllipse))
                    return ((ZoneEllipse)zoneGeometry).X0;
                if (zoneGeometry.GetType() == typeof(ZoneHalfPlane))
                    return ((ZoneHalfPlane)zoneGeometry).X0;
                else throw new Exception();
            }
            set 
            {
                if (zoneGeometry.GetType() == typeof(ZoneEllipse))
                    ((ZoneEllipse)zoneGeometry).X0 = value;
                else if (zoneGeometry.GetType() == typeof(ZoneHalfPlane))
                    ((ZoneHalfPlane)zoneGeometry).X0 = value;
                else throw new Exception();
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
            get { return ((ZoneEllipse)zoneGeometry).Phi; }
            set { ((ZoneEllipse)zoneGeometry).Phi = value;  }
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
    }

    public class ComplexZoneRelay
    {
        private Zone[] zones = new Zone[4];
        private ZoneCombinations zoneCombination = new ZoneCombinations();

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
            zoneCombination.ZoneCombinationChanged += OnZoneCombinationChanged;
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
                return zoneCombination;
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
}

using System;
using System.Collections.Generic;
using System.Text;
using System.Windows;
using System.Windows.Controls;

namespace RastrChartWPFControl
{
    class ChartMarkerRepel
    {
        class MarkerInfo : IComparable
        {
            public ChartChannel channel;
            public Point pointerBegin;
            public Point pointerEnd;
            public double yValue;
            public Size size;

            public MarkerInfo(ChartChannel Channel, Point PointerBegin, double Yvalue)
            {
                channel = Channel;
                pointerEnd = pointerBegin = PointerBegin;
                yValue = Yvalue;
                size = channel.SetInfoWindowValue(yValue);
            }

            public int CompareTo(object exMarker)
            {
                double dDiff = pointerBegin.Y - ((MarkerInfo)exMarker).pointerBegin.Y;
                if (dDiff > 0) return 1;
                if (dDiff < 0) return -1;
                return 0;
            }
        }

        private Canvas canvasToMeasure;
        private List<MarkerInfo> WindowList = new List<MarkerInfo>();
        private double maxWidth;

        public ChartMarkerRepel(Canvas CanvasToMeasure)
        {
            canvasToMeasure = CanvasToMeasure;
        }

        public void AddWindow(ChartChannel Channel, Point PointerBegin, double yValue)
        {
            WindowList.Add(new MarkerInfo(Channel, PointerBegin, yValue));
        }

        public void Arrange(Canvas ParentCanvas)
        {
            GetPlacement();

            foreach (MarkerInfo marker in WindowList)
            {
                Point pb = canvasToMeasure.PointFromScreen(marker.pointerBegin);
                Point pe = canvasToMeasure.PointFromScreen(marker.pointerEnd);
                marker.channel.UpdateInfoWindow(pe, pb, pe, marker.yValue);
                marker.channel.InfoWindowOpen = true;
            }
        }

        private void ProcessGroupedList(int groupListIndex, List<MarkerInfo> GroupList)
        {
            if (GroupList.Count > 1)
            {
                double yCenter = 0.0;
                int mid = GroupList.Count / 2;
                if (GroupList.Count % 2 == 0)
                {
                    yCenter = (GroupList[mid - 1].yValue + GroupList[mid].yValue) * 0.5;
                }
                else
                {
                    yCenter = GroupList[mid].yValue;
                }

                for (int k = 0; k < GroupList.Count; k++)
                {
                    MarkerInfo mi = WindowList[k + groupListIndex];
                    double constCos = (1 - Math.Cos(2 * Math.PI / GroupList.Count));
                    double rGuard = 2.0;
                    double diag = rGuard * (maxWidth * maxWidth + mi.size.Height * mi.size.Height);
                    mi.pointerEnd.X = mi.pointerBegin.X + Math.Sqrt(diag / constCos) * Math.Cos(2 * Math.PI * k / GroupList.Count);
                    mi.pointerEnd.Y = mi.pointerBegin.Y - Math.Sqrt(diag / constCos) * Math.Sin(2 * Math.PI * k / GroupList.Count);
                }
                GroupList.Clear();
            }
        }

        protected void GetPlacement()
        {
            WindowList.Sort();
            List<MarkerInfo> GroupList = new List<MarkerInfo>();

            int groupListIndex = 0;

            maxWidth = 0.0;
            foreach(MarkerInfo mi in WindowList)
            {
                if (maxWidth < mi.size.Width)
                    maxWidth = mi.size.Width;
            }

            for(int i = 1 ; i < WindowList.Count; i++)
            {
                double diff = WindowList[i].pointerBegin.Y - WindowList[i - 1].pointerBegin.Y;
                if(diff < WindowList[0].size.Height)
                {
                    if (!GroupList.Contains(WindowList[i - 1]))
                        GroupList.Add(WindowList[i-1]);
                    GroupList.Add(WindowList[i]);
                }
                else
                {
                    ProcessGroupedList(groupListIndex, GroupList);
                    groupListIndex = i;
                }
            }
            ProcessGroupedList(groupListIndex, GroupList);
        }
    }
}

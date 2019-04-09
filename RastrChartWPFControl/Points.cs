using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;

namespace RastrChartWPFControl
{
    internal enum PointsChangeType
    {
        Added,
        AddedBulk,
        Cleared,
        Replaced,
        Sorted
    }

    internal class PointsEventArgs : EventArgs
    {
        public PointsChangeType pointChangeType;
        internal PointsEventArgs(PointsChangeType ChangeType)
        {
            pointChangeType = ChangeType;
        }
    }

    public class Points
    {
        internal event EventHandler PointsChanged;
        internal Point pointAdded;
        internal Point[] pointsAdded;

        public void Add(double x, double y)
        {
            Add(new Point(x, y));            
        }

        public void Add(Point point)
        {
            pointAdded = point;
            OnPointsChanged(PointsChangeType.Added);
        }

        /// <summary>
        ///  Method adds points from array by concatenating them with current points
        /// </summary>
        public void Add(Point[] points)
        {
            pointsAdded = points;
            OnPointsChanged(PointsChangeType.AddedBulk);
        }

        public void Add(double[,] points)
        {
            pointsAdded = new Point[points.GetLength(0)];
            int nCount = 0;
            for (int i = points.GetLowerBound(0); i <= points.GetUpperBound(0); i++ )
            {
                pointsAdded[nCount].X = points[i, 1];
                pointsAdded[nCount++].Y = points[i, 0];
            }
            OnPointsChanged(PointsChangeType.AddedBulk);
        }

        public void Replace(Point[] points)
        {
            pointsAdded = points;
            OnPointsChanged(PointsChangeType.Replaced);
        }

        public void Clear()
        {
            OnPointsChanged(PointsChangeType.Cleared);
        }

        public void SortX()
        {
            OnPointsChanged(PointsChangeType.Sorted);
        }

        internal void OnPointsChanged(PointsChangeType ChangeType)
        {
            if (PointsChanged != null)
                PointsChanged(this, new PointsEventArgs(ChangeType));
        }
    }
}

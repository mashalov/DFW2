using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Controls;

namespace RastrChartWPFControl
{
    public class RulerEventArgs : EventArgs
    {
        private double newPosition, oldPosition;
        private Point coordinates;
        private string tag;
        private double radius;

        public double Radius
        {
            get { return radius; }
            set { radius = value; }
        }

        public string Tag
        {
            get { return tag; }
        }
        public double NewPosition
        {
            get { return newPosition; }
        }

        public double OldPosition
        {
            get { return oldPosition; }
        }

        public Point Coordinates
        {
            get { return coordinates; }
            set { coordinates = value; }
        }

        public bool Cancel
        {
            get;
            set;
        }


        public RulerEventArgs(string RulerTag, double RulerPositionOld, double RulerPositionNew)
            : base()
        {
            tag = RulerTag;
            newPosition = RulerPositionNew;
            oldPosition = RulerPositionOld;
            Cancel = false;
            radius = 0;
        }
    }

    public delegate void RulerPositionHandler(object sender, RulerEventArgs e);

    public enum RulerTipOrientation
    {
        RTOTop,
        RTOBottom,
        RTORight,
        RTOLeft
    }

    public class RulerTip : FrameworkElement
    {
        private FontFamily ff;
        private Typeface ft;
        private string text = "";
        TextBlock tb = new TextBlock();
        private double AddWidth  = 10;
        private double AddHeight = 5;
        private double TipSize = 5;
        private double TipCatet;
        private RulerTipOrientation orientation;
        private Color color;
        private double tipoffset = 0;

        public RulerTip()
        {
            ff = tb.FontFamily;
            ft = new Typeface(ff, tb.FontStyle, tb.FontWeight, tb.FontStretch);
            TipCatet = TipSize * 0.70710678118654752440084436210485;
        }   

        public string Text
        {
            get { return text; }
            set 
            { 
                tb.Text = text = value;
                InvalidateVisual();
                InvalidateMeasure();
            }
        }

        public Color TipColor
        {
            get { return color; }
            set
            {
                color = value; 
                InvalidateVisual();
            }
        }

        public RulerTipOrientation Orientation
        {
            get
            {
                return orientation;
            }
            set
            {
                orientation = value;
                InvalidateVisual();
                InvalidateMeasure();
            }
        }

        public void CalculateOrientation(RulerTipOrientation BaseOrientation, double Distance, double ParentWidth)
        {
            tipoffset = 0;
            orientation = BaseOrientation;
            switch(BaseOrientation)
            {
                case RulerTipOrientation.RTOBottom:
                double Width = DesiredSize.Width;
                if (Distance <= 2 * TipCatet)
                    orientation = RulerTipOrientation.RTOLeft;
                else
                    if (Distance < Width / 2)
                        tipoffset = Width / 2 - Distance;
                    else
                        if (Distance >= ParentWidth - 2 * TipCatet)
                            orientation = RulerTipOrientation.RTORight;
                        else
                            if (Distance > ParentWidth - Width / 2)
                                tipoffset = ParentWidth - Width /2 - Distance;
                break;
                case RulerTipOrientation.RTOLeft:
                double Height = DesiredSize.Height;
                if (Distance <= 2 * TipCatet)
                    orientation = RulerTipOrientation.RTOTop;
                else
                    if (Distance < Height / 2)
                        tipoffset = Height / 2 - Distance;
                    else
                        if (Distance >= ParentWidth - 2 * TipCatet)
                            orientation = RulerTipOrientation.RTOBottom;
                        else
                            if (Distance > ParentWidth - Height / 2)
                                tipoffset = ParentWidth - Height /2 - Distance;
                break;
            }
        }

        protected Geometry GetBaloon(Size size)
        {
            Rect baseRect = new Rect(1, 1, size.Width + AddWidth - 2, size.Height + AddHeight - 1);

            

            TranslateTransform tt = null;
            switch (orientation)
            {
                case RulerTipOrientation.RTOTop:
                    baseRect.Offset(0,TipCatet);
                    tt = new TranslateTransform(baseRect.Width / 2, 1);
                    break;
                case RulerTipOrientation.RTOBottom:
                    tt = new TranslateTransform(baseRect.Width / 2 - tipoffset, baseRect.Height - TipCatet + 1);
                    break;
                case RulerTipOrientation.RTOLeft:
                    baseRect.Offset(TipCatet, 0);
                    tt = new TranslateTransform(TipCatet + 1, baseRect.Height / 2 - TipCatet - tipoffset);
                    break;
                case RulerTipOrientation.RTORight:
                    tt = new TranslateTransform(baseRect.Width + 1, baseRect.Height / 2 - TipCatet);
                    break;
                default:
                    tt = new TranslateTransform(0, 0);
                    break;

            }

            CombinedGeometry combgeo = new CombinedGeometry();
            combgeo.GeometryCombineMode = GeometryCombineMode.Union;
            RectangleGeometry rcgeo = new RectangleGeometry(baseRect, 2, 2);
            TransformGroup tiptrans = new TransformGroup();
            RotateTransform rt = new RotateTransform(45);
            tiptrans.Children.Add(rt);
            tiptrans.Children.Add(tt);
            RectangleGeometry tipgeo = new RectangleGeometry(new Rect(0, 0, TipSize, TipSize), 0, 0, tiptrans);
            combgeo.Geometry1 = rcgeo;
            combgeo.Geometry2 = tipgeo;
            
            return combgeo;
        }

        protected override void OnRender(DrawingContext drawingContext)
        {
            base.OnRender(drawingContext);

            FormattedText ft2 = new FormattedText(text,
                                                 System.Globalization.CultureInfo.CurrentCulture,
                                                 System.Windows.FlowDirection.LeftToRight,
                                                 ft,
                                                 tb.FontSize,
                                                 Brushes.Black,
                                                 System.Windows.Media.VisualTreeHelper.GetDpi(this).PixelsPerDip);
            tb.Text = text;
            tb.Measure(new Size(Double.PositiveInfinity,Double.PositiveInfinity));

            Color clr = color;
            clr.A = 128;
            Color whiteTrans = Colors.White;
            clr.A = 10;
            LinearGradientBrush lgb = new LinearGradientBrush(clr, whiteTrans, 90);
            lgb.Freeze();

            drawingContext.DrawGeometry(lgb, new Pen(new SolidColorBrush(color), 1), GetBaloon(tb.DesiredSize));
            Point textPoint = new Point(0.5 * AddWidth, 0.25 * AddHeight);
            switch (orientation)
            {
                case RulerTipOrientation.RTOLeft:
                    textPoint.X += TipCatet;
                    break;
                case RulerTipOrientation.RTOTop:
                    textPoint.Y += TipCatet;
                    break;
            }
            // new Point(0.5 * AddWidth + ((orientation==RulerTipOrientation.RTOLeft)?TipCatet:0), 0.5 * AddHeight / 2)
            drawingContext.DrawText(ft2, textPoint);
        }

        protected override Size MeasureOverride(Size availableSize)
        {
            base.MeasureOverride(availableSize);
            tb.Measure(availableSize);
            return new Size(tb.DesiredSize.Width + AddWidth + TipCatet, tb.DesiredSize.Height + AddHeight + TipCatet);
        }
    }

    public enum RulerOrientation
    {
        RulerHorizontal,
        RulerVertical,
        RulerSpot
    }

    public abstract class RulerBase
    {
        protected double position;
        protected Color rulerColor = Colors.Red;
        protected Point dragPoint;
        private string tag;

        public event RulerPositionHandler RulerPosition;
        public event RulerPositionHandler RulerAskCoordinates;

        public RulerBase()
        {
            position = 0;
        }

        protected void OnRulerPosition(double OldPosition, double NewPosition)
        {
            if (RulerPosition != null)
                RulerPosition(this, new RulerEventArgs(tag,OldPosition,NewPosition));
        }

        protected bool AskCoordinates(ref Point point, ref double Radius)
        {
            bool bRes = false;
            if (RulerAskCoordinates != null)
            {
                RulerEventArgs rulerEvent = new RulerEventArgs(Tag,position,position);
                RulerAskCoordinates(this, rulerEvent);
                point = rulerEvent.Coordinates;
                bRes = !rulerEvent.Cancel;
                Radius = rulerEvent.Radius;
            }
            return bRes;
        }

        public double Position
        {
            get { return position; }
            set
            {
                if (value != position)
                {
                    double OldPos = position;
                    position = value;
                    OnRulerPosition(OldPos, value);
                }
            }
        }

        protected virtual void OnRulerColor()
        {

        }

        public Color RulerColor
        {
            get { return rulerColor; }
            set
            {
                if (value != rulerColor)
                {
                    rulerColor = value;
                    OnRulerColor();
                }
            }
        }

        public string Tag
        {
            get { return tag; }
            set { tag = value; }
        }

        public virtual double GetHitTestDistance(Point ScreenPoint)
        {
            return Double.PositiveInfinity;
        }
        
        abstract public void PlaceRuler(Canvas PlaceCanvas, Point ZoomFactor, Point Offset);

        abstract public RulerOrientation Orientation
        {
            get;
        }

        public abstract void BeginDrag(Point ScreenPoint);
        public abstract void Drag(Point ScreenPoint, Point ZoomFactor, Point Offset);
    }

    abstract class RulerLine : RulerBase
    {
        protected Line rulerLine = null;
        protected Canvas infoCanvas;
        protected RulerTip infoBlock = null;

        public RulerLine(Canvas InfoCanvas) : base()
        {
            infoCanvas = InfoCanvas;
            position = 0;
            rulerLine = new Line();
            rulerLine.StrokeThickness = 3;
            rulerLine.SnapsToDevicePixels = true;
            rulerLine.Stroke = new SolidColorBrush(rulerColor);
            rulerLine.Opacity = .6;
            infoBlock = new RulerTip();
            infoCanvas.Children.Add(infoBlock);
            infoBlock.TipColor = rulerColor;
        }

        protected override void OnRulerColor()
        {
            rulerLine.Stroke = new SolidColorBrush(rulerColor);
            infoBlock.TipColor = rulerColor;
        }

        public override double GetHitTestDistance(Point ScreenPoint)
        {
            double dx = rulerLine.X2 - rulerLine.X1;
            double dy = rulerLine.Y2 - rulerLine.Y1;
            double ddy = rulerLine.Y1 - ScreenPoint.Y;
            double ddx = rulerLine.X1 - ScreenPoint.X;
            double z = Math.Sqrt(dx * dx + dy * dy);
            if (z > 0)
                return Math.Abs(dx * ddy - ddx * dy) / z;
            else
                return Double.PositiveInfinity;
        }

        public override void BeginDrag(Point ScreenPoint)
        {
            if (rulerLine != null)
            {
                dragPoint.X = rulerLine.X1 - ScreenPoint.X;
                dragPoint.Y = rulerLine.Y1 - ScreenPoint.Y;
            }
        }

        public override void Drag(Point ScreenPoint, Point ZoomFactor, Point Offset)
        {
            UpdateInfoBlock();
        }

        protected virtual void UpdateInfoBlock()
        {

        }
    }

    class RulerHorizontal : RulerLine
    {
        public RulerHorizontal(Canvas InfoCanvas) : base(InfoCanvas)
        {

        }

        public override void PlaceRuler(Canvas PlaceCanvas, Point ZoomFactor, Point Offset)
        {
            rulerLine.X1 = 0;
            rulerLine.X2 = 10000;
            if (!Double.IsNaN(ZoomFactor.Y) && !Double.IsNaN(Offset.Y))
            {
                rulerLine.Y1 = rulerLine.Y2 = -position * ZoomFactor.Y + Offset.Y;
                PlaceCanvas.Children.Add(rulerLine);
                UpdateInfoBlock();
            }
        }

        public override void Drag(Point ScreenPoint, Point ZoomFactor, Point Offset)
        {
            rulerLine.Y1 = rulerLine.Y2 = ScreenPoint.Y + dragPoint.Y;
            Position = -(rulerLine.Y1 - Offset.Y) / ZoomFactor.Y;
            base.Drag(ScreenPoint,ZoomFactor,Offset);
        }

        public override RulerOrientation Orientation
        {
            get { return RulerOrientation.RulerHorizontal; }
        }

        protected override void UpdateInfoBlock()
        {
            infoBlock.Text = position.ToString("G5");
            infoBlock.Measure(new Size(Double.PositiveInfinity, Double.PositiveInfinity));
            double InfoBlockHeight = infoBlock.DesiredSize.Height;
            Canvas.SetLeft(infoBlock, 3.0);
            double ofsy = rulerLine.Y1 - (InfoBlockHeight - 3) / 2;
            infoBlock.CalculateOrientation(RulerTipOrientation.RTOLeft, rulerLine.Y1, infoCanvas.ActualHeight);
            if (ofsy < 0)
                ofsy = 0;
            if (ofsy > infoCanvas.ActualHeight - InfoBlockHeight)
                ofsy = infoCanvas.ActualHeight - InfoBlockHeight;
            Canvas.SetTop(infoBlock, ofsy);
        }
    }


    internal class RulerSpot : RulerBase
    {
        private Ellipse elpI = new Ellipse();
        private Ellipse elpO = new Ellipse();

        public RulerSpot() : base()
        {
            elpI.Fill = Brushes.Green;
            elpO.Stroke = Brushes.LightGreen;
            elpO.Fill = new SolidColorBrush(Color.FromArgb(50, 0, 255, 0));
            elpO.StrokeThickness = 3;
            elpI.Width = elpI.Height = 8;
            elpO.Width = elpO.Height = 20;
        }

        public override void PlaceRuler(Canvas PlaceCanvas, Point ZoomFactor, Point Offset)
        {
            if (!Double.IsNaN(ZoomFactor.Y) && !Double.IsNaN(Offset.Y))
            {
                Point point = new Point();
                double Radius = 0;

                if (AskCoordinates(ref point, ref Radius))
                {
                    point.X = point.X * ZoomFactor.X + Offset.X;
                    point.Y = -point.Y * ZoomFactor.Y + Offset.Y;

                    if (Radius == 0)
                        Radius = 20;
                    else
                    {
                        Radius *= ZoomFactor.X;
                        if (Radius < 8)
                            Radius = 8;
                    }

                    elpO.Width = elpO.Height = Radius;

                    Canvas.SetLeft(elpI, point.X - elpI.Width  / 2);
                    Canvas.SetTop(elpI,  point.Y - elpI.Height / 2);
                    Canvas.SetLeft(elpO, point.X - elpO.Width  / 2);
                    Canvas.SetTop(elpO,  point.Y - elpO.Height / 2);

                    PlaceCanvas.Children.Add(elpI);
                    PlaceCanvas.Children.Add(elpO);
                }
            }
        }

        public override void BeginDrag(Point ScreenPoint)
        {
            throw new NotImplementedException();
        }

        public override void Drag(Point ScreenPoint, Point ZoomFactor, Point Offset)
        {
            throw new NotImplementedException();
        }

        public override RulerOrientation Orientation
        {
            get { return RulerOrientation.RulerSpot; }
        }
    }

    class RulerVertical : RulerLine
    {
        public RulerVertical(Canvas InfoCanvas) : base(InfoCanvas)
        {

        }

        public override void Drag(Point ScreenPoint, Point ZoomFactor, Point Offset)
        {
            rulerLine.X1 = rulerLine.X2 = ScreenPoint.X + dragPoint.X;
            Position = Math.Max((rulerLine.X1 - Offset.X) / ZoomFactor.X,0.0);
            base.Drag(ScreenPoint, ZoomFactor, Offset);
        }

        public override void PlaceRuler(Canvas PlaceCanvas, Point ZoomFactor, Point Offset)
        {
            rulerLine.Y1 = 0;
            rulerLine.Y2 = 10000;
            if (!Double.IsNaN(ZoomFactor.X) && !Double.IsNaN(Offset.X))
            {
                rulerLine.X1 = rulerLine.X2 = position * ZoomFactor.X + Offset.X;
                PlaceCanvas.Children.Add(rulerLine);
                UpdateInfoBlock();
            }
        }

        protected override void UpdateInfoBlock()
        {
            infoBlock.Text = position.ToString("G5");
            infoBlock.Measure(new Size(Double.PositiveInfinity, Double.PositiveInfinity));
            double InfoBlockWidth = infoBlock.DesiredSize.Width;
            Canvas.SetTop(infoBlock, (infoCanvas.ActualHeight - infoBlock.DesiredSize.Height) / 2);
            double ofs = rulerLine.X1 - (InfoBlockWidth - 5) / 2;
            infoBlock.CalculateOrientation(RulerTipOrientation.RTOBottom, rulerLine.X1, infoCanvas.ActualWidth);
            if (ofs < 0)
                ofs = 0;
            if (ofs > infoCanvas.ActualWidth - InfoBlockWidth)
                ofs = infoCanvas.ActualWidth - InfoBlockWidth;
            Canvas.SetLeft(infoBlock, ofs);
        }

        public override RulerOrientation Orientation
        {
            get { return RulerOrientation.RulerVertical; }
        }
    }
}


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
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace RastrChartWPFControl
{
    
    /// <summary>
    /// Interaction logic for ChartCanvas.xaml
    /// </summary>
    /// 

    public partial class RastrChart : UserControl
    {
        private ChartCanvasPlotter plotter;
        private ChannelCollection channels;

        public event DragEventHandler DataDrop;
        public event SetReferenceValueEvent SetReferenceValue;
        public event SetReferenceValueEvent ChannelPin;
        public event EventHandler SaveChartSet;
        public event SetReferenceValueEvent ChannelDelete;
        public event SetReferenceValueEvent ChannelPropsChanged;

        public static string strPngFileMask = "PNG-файл (.png)|*.png";
        
        public RastrChart()
        {
            InitializeComponent();
            plotter = new ChartCanvasPlotter(ChannelCanvas, ZoomCanvas, MeasureCanvas,HorizontalAxis, VerticalAxis, LegendArea);
            channels = new ChannelCollection(plotter);
            plotter.RenderBitmap += OnRenderBitmap;
            plotter.AskCopyToClipBoard += OnClipboardCopyBitmap;
            plotter.SetReferenceValue += OnSetReferenceValue;
            plotter.ChannelPin += OnChannelPin;
            plotter.ObtainFocus += OnObtainFocus;
            plotter.SaveChartSet += OnSaveChartSet;
            plotter.ChannelDelete += OnChannelDelete;
            plotter.ChannelPropsChanged += OnChannelPropsChanged;
            Unloaded += RastrChart_Unloaded;

            RoutedCommand AltC = new RoutedCommand();
            AltC.InputGestures.Add(new KeyGesture(Key.C, ModifierKeys.Alt));
            CommandBindings.Add(new CommandBinding(AltC, OnAltC));

            RoutedCommand AltS = new RoutedCommand();
            AltS.InputGestures.Add(new KeyGesture(Key.S, ModifierKeys.Alt));
            CommandBindings.Add(new CommandBinding(AltS, OnAltS));

        }

        private void RastrChart_Unloaded(object sender, RoutedEventArgs e)
        {
            Unloaded -= RastrChart_Unloaded;
            plotter.RenderBitmap -= OnRenderBitmap;
            plotter.AskCopyToClipBoard -= OnClipboardCopyBitmap;
            plotter.SetReferenceValue -= OnSetReferenceValue;
            plotter.ChannelPin -= OnChannelPin;
            plotter.ObtainFocus -= OnObtainFocus;
            plotter.SaveChartSet -= OnSaveChartSet;
            plotter.ChannelDelete -= OnChannelDelete;
            plotter.ChannelPropsChanged -= OnChannelPropsChanged;
            channels.RemoveAll();
            ChannelCanvas.Children.Clear();
            ZoomCanvas.Children.Clear();
            MeasureCanvas.Children.Clear();
            HorizontalAxis.Children.Clear();
            VerticalAxis.Children.Clear();
            LegendArea.Children.Clear();
            plotter.CleanUp();
        }

        protected void OnClipboardCopyBitmap(object sender, EventArgs e)
        {
            CopyUIElementToClipboard(this);
        }

        protected void OnRenderBitmap(RenderBitmapEventArgs e)
        {
            e.bmp = CopyUIElementToBitmap(this);
        }


        private RenderTargetBitmap CopyUIElementToBitmap(FrameworkElement element)
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
            return bmpCopied;
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

        protected void SaveToPNG(FrameworkElement element)
        {
            try
            {
                Point ptlu = element.PointToScreen(new Point(0, 0));
                using (System.Drawing.Bitmap bmp = new System.Drawing.Bitmap((int)element.ActualWidth, (int)element.ActualHeight))
                {
                    using (System.Drawing.Graphics g = System.Drawing.Graphics.FromImage(bmp))
                    {
                        g.CopyFromScreen((int)ptlu.X, (int)ptlu.Y, 0, 0, bmp.Size);
                        plotter.CancelMouse();
                        string pngExportPath = "c:\\tmp\\";
                        Microsoft.Win32.SaveFileDialog saveDialog = new Microsoft.Win32.SaveFileDialog();
                        saveDialog.InitialDirectory = System.IO.Path.GetDirectoryName(pngExportPath);
                        saveDialog.AddExtension = true;
                        saveDialog.FileName = System.IO.Path.GetFileNameWithoutExtension(pngExportPath);
                        saveDialog.Filter = strPngFileMask;
                        if (saveDialog.ShowDialog() == true)
                            bmp.Save(saveDialog.FileName);
                    }
                }

                
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private static void TakeScreenShot(FrameworkElement element)
        {

            try
            {
                Point ptlu = element.PointToScreen(new Point(0, 0));
                using (System.Drawing.Bitmap bmp = new System.Drawing.Bitmap((int)element.ActualWidth, (int)element.ActualHeight))
                {
                    using (System.Drawing.Graphics g = System.Drawing.Graphics.FromImage(bmp))
                    {
                        g.CopyFromScreen((int)ptlu.X, (int)ptlu.Y, 0, 0, bmp.Size);
                        System.Windows.Forms.Clipboard.SetImage(bmp);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }
      

        public void ZoomExtents()
        {
            plotter.ZoomExtents();
        }

        public void Flash(bool bHighlight)
        {
            plotter.Flash(bHighlight);
        }

        public double TimeMarker
        {
            set { plotter.TimeMarker = value; } 
        }

        public void AddMarker(int Type, int SubType, int Id, string Name, double Time)
        {
            if(Name == "")
            {
                Name = string.Format("{0}", Id);
            }
            plotter.AddEventMarker(Name, Time);
        }

        public void ClearMarkers()
        {
            plotter.ClearEventMarkers();
        }

        public AxisConstraints AxisXConstraints
        {
            get { return plotter.AxisXConstraints; }
            set { plotter.AxisXConstraints = value; }
        }


        public AxisConstraints AxisYConstraints
        {
            get { return plotter.AxisYConstraints; }
            set { plotter.AxisYConstraints = value; }
        }

        public Size PlotSize
        {
            get { return plotter.PlotSize;  }
        }

        
        public ChannelCollection Channels
        {
            get
            {
                return channels;
            }
        }
        
        private void OnDragEnter(object sender, DragEventArgs e)
        {
            e.Effects= DragDropEffects.Copy;
        }

        private void OnDragOver(object sender, DragEventArgs e)
        {
            e.Effects = DragDropEffects.Copy;
        }

        private void OnDrop(object sender, DragEventArgs e)
        {
            OnDataDrop(sender,e);
        }

        private void OnDataDrop(object sender, DragEventArgs e)
        {
            if(DataDrop != null)
                DataDrop(sender,e);
        }

        private void OnSetReferenceValue(SetReferenceValueEventArgs args)
        {
            if (SetReferenceValue != null)
                SetReferenceValue(args);
        }

        private void OnChannelPin(SetReferenceValueEventArgs args)
        {
            if (ChannelPin != null)
                ChannelPin(args);
        }

        private void OnChannelDelete(SetReferenceValueEventArgs args)
        {
            if (ChannelDelete != null)
                ChannelDelete(args);
        }

        private void OnChannelPropsChanged(SetReferenceValueEventArgs args)
        {
            if (ChannelPropsChanged != null)
                ChannelPropsChanged(args);
        }

        private void OnSaveChartSet(object sender, EventArgs e)
        {
            if (SaveChartSet != null)
                SaveChartSet(sender,e);
        }

        private void OnObtainFocus(object sender, EventArgs e)
        {
             Focus();
        }

        private void OnAltC(object sender, ExecutedRoutedEventArgs e)
        {
            TakeScreenShot(this);
        }

        private void OnAltS(object sender, ExecutedRoutedEventArgs e)
        {
            SaveToPNG(this);
        }

        private void LostMouse(object sender, MouseEventArgs e)
        {
            plotter.CancelMouse();
        }
    }
}

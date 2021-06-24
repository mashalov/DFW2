using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Reflection;

namespace ResultFileTest
{
    public partial class Form1 : Form
    {
        private ResultFileLib.Result resultFile;
        private ResultFileLib.ResultRead resultRead;
        private DeviceTreeNodePool nodePool = new DeviceTreeNodePool();
        ResultFileLib.Device root;
        DeviceTreeNode rootNode;
        private ResultFileLib.Variable variableDrag;

        private RastrChartWPFControl.RastrChart rastrChart;
        private System.Windows.Forms.Integration.ElementHost chartHost;


        private void OnChartDrop(object sender, System.Windows.DragEventArgs e)
        {
            try
            {
                if(variableDrag != null)
                {
                    object s = e.Data.GetData("RastrDND");
                    string legend = string.Format("{0} {1}", variableDrag.Device.Name, variableDrag.Name);
                    RastrChartWPFControl.ChartChannel channel = rastrChart.Channels.Add(legend);
                    channel.Units = variableDrag.UnitsName;
                    double [,] points = variableDrag.Plot;
                    channel.Points.Add(points);
                    rastrChart.ZoomExtents();
                }
                else
                {
                    RastrChartWPFControl.ChartChannel channel = rastrChart.Channels.Add("Шаг");
                    channel.Units = "c";
                    double[,] points = resultRead.TimeStep;
                    channel.Points.Add(points);
                    rastrChart.ZoomExtents();
                }
            }
            catch(Exception ex)
            {
                System.Windows.MessageBox.Show(ex.Message,"Ошибка",System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Exclamation);
            }
        }


        public Form1()
        {
            InitializeComponent();
        }

        public void FilterTree(string Filter, bool FilterMode)
        {
            try
            {
                DeviceTree.BeginUpdate();
                nodePool.FilterTree(Filter, FilterMode);
            }
            finally
            {
                DeviceTree.EndUpdate();
            }
        }

        public void PopulateTree()
        {
            resultFile = new ResultFileLib.Result();
            resultRead = resultFile.Load("c:\\tmp\\binresultcom.rst");
            //resultRead = resultFile.Load("c:\\tmp\\000023.sna");
            root = resultRead.Root;
            Stack<DeviceTreeNode> stack = new Stack<DeviceTreeNode>();

            try
            {
                DeviceTree.BeginUpdate();
                rootNode = nodePool.NewDeviceTreeNode(root);
                DeviceTree.Nodes.Add(rootNode);
                rootNode.Populate();
                rootNode.Expand();
                Text = $"Version {resultRead.Version} Channels {resultRead.Channels} Points {resultRead.Points} Ratio {resultRead.CompressionRatio}";
            }
            finally
            {
                DeviceTree.EndUpdate();
            }

           // resultFile.ExportCSV("c:\\tmp\\juuu.csv");

            return;
        }

        private void OnLoad(object sender, EventArgs e)
        {
            VarBox.Columns.Add("Имя");
            VarBox.Columns.Add("Единицы");

            chartHost = new System.Windows.Forms.Integration.ElementHost();
            chartHost.Dock = DockStyle.Fill;
            ChartContainer.Panel2.Controls.Add(chartHost);

            rastrChart = new RastrChartWPFControl.RastrChart();
            rastrChart.InitializeComponent();
            chartHost.Child = rastrChart;

            rastrChart.DataDrop += OnChartDrop;

            PopulateTree();

          //  resultFile.ExportCSV("c:\\tmp\\result.csv");
        }

        private void OnTextChanged(object sender, EventArgs e)
        {
            FilterTree(FilterBox.Text, FilterBox.FilterMode);
        }

        private void OnDrawNode(object sender, DrawTreeNodeEventArgs e)
        {
            // Draw the background and node text for a selected node.

            DeviceTreeNode DevTreeNode = (DeviceTreeNode)e.Node;

            if (!e.Node.IsVisible) return;

            Rectangle textBounds = e.Bounds;
            textBounds.Offset(1, 1);
            textBounds.Width = DeviceTree.Width;
            Rectangle focusBounds = e.Bounds;
            focusBounds.Width--;
            focusBounds.Height--;
            Rectangle backBounds = e.Bounds;
            e.Graphics.FillRectangle(DevTreeNode.WindowBrush, backBounds);
            Font nodeFont = e.Node.NodeFont;
            if (nodeFont == null) nodeFont = ((TreeView)sender).Font;

            if ((e.State & TreeNodeStates.Selected) != 0)
            {
                e.Graphics.FillRectangle(DevTreeNode.HighLightBrush, backBounds);
                TextRenderer.DrawText(e.Graphics,e.Node.Text,nodeFont,new Point(textBounds.Left, textBounds.Top), Color.FromName("White"));
            }
            else
            {
                string Text = e.Node.Text;
                foreach(TextHighLightPair thp in DevTreeNode.HighLightPairs)
                {
                    string LeftSubstr = Text.Substring(0, thp.from);
                    Size sz = new Size();
                    TextFormatFlags txff = TextFormatFlags.Left | TextFormatFlags.NoClipping | TextFormatFlags.NoPadding;
                    int LeftBlock = TextRenderer.MeasureText(e.Graphics, LeftSubstr, nodeFont, sz, txff).Width;
                    LeftSubstr = Text.Substring(thp.from, thp.to - thp.from);
                    int Width = TextRenderer.MeasureText(e.Graphics, LeftSubstr, nodeFont, sz, txff).Width;
                    e.Graphics.FillRectangle(Brushes.Yellow, new Rectangle(LeftBlock + textBounds.Left, backBounds.Top, Width, backBounds.Height));
                }
                
                TextRenderer.DrawText(e.Graphics, e.Node.Text, nodeFont, new Point(textBounds.Left, textBounds.Top), Color.FromName("Black"));
            }

            if((e.State & TreeNodeStates.Focused) != 0)
            {
                using (Pen focusPen = new Pen(Color.Black))
                {
                    focusPen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dot;
                    e.Graphics.DrawRectangle(focusPen, focusBounds);
                }
            }
        }

        private void OnItemSelect(object sender, TreeViewEventArgs e)
        {
            DeviceTreeNode devTreeNode = (DeviceTreeNode)e.Node;
            if (devTreeNode.Parent == null)
            {
                VarBox.Items.Clear();
                VarBox.Items.Add(new ModelVariableItem());
            }
            else
            {
                try
                {
                    VarBox.BeginUpdate();
                    VarBox.Items.Clear();
                    ResultFileLib.Variables vars = devTreeNode.Variables;
                    foreach (ResultFileLib.Variable variable in vars)
                    {
                        VarBox.Items.Add(new DeviceVariableItem(variable));
                    }
                }
                finally
                {
                    VarBox.EndUpdate();
                }
            }
        }

        private void OnVariableDrag(object sender, ItemDragEventArgs e)
        {
            if (e.Item is ModelVariableItem)
            {
                variableDrag = null;
                DataObject dataObj = new DataObject();
                const string FormatType = "RastrDND";
                dataObj.SetData(FormatType, false, "");
                DoDragDrop(dataObj, DragDropEffects.Copy);
            }
            else if (e.Item is DeviceVariableItem)
            {
                variableDrag = ((DeviceVariableItem)e.Item).Variable;
                DataObject dataObj = new DataObject();
                const string FormatType = "RastrDND";
                dataObj.SetData(FormatType, false, "");
                DoDragDrop(dataObj, DragDropEffects.Copy);
            }
        }
    }
}

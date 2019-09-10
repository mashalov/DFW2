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
    public class ChannelCollection
    {
        private ChartCanvasPlotter plotter;
       
        internal ChannelCollection(ChartCanvasPlotter ChartPlotter)
        {
            plotter = ChartPlotter;
        }
        public ChartChannel Add(string Legend)
        {
            return plotter.AddChannel(Legend);
        }
        public ChartChannel this[int index]
        {
            get { return plotter.Channels[index]; }
        }

        public ChartChannel this[string Tag]
        {
            get { return plotter.GetChannelByTag(Tag); }
        }

        public ChartChannel ByLegend(string Name)
        {
            return plotter.GetChannelByLegend(Name);
        }

        public int Count
        {
            get { return plotter.Channels.Count; }
        }
        public void RemoveAll()
        {
            plotter.RemoveAllChannels();
        }
        public List<ChartChannel>.Enumerator GetEnumerator()
        {
            return plotter.Channels.GetEnumerator();
        }

        public void SortByTags()
        {
            plotter.SortChannelsByTags();
        }
    }
}
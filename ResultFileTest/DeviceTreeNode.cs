using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;


namespace ResultFileTest
{
    class DeviceTreeNodeStack : Stack<DeviceTreeNode> { }
    class DeviceTreeNodeList  : List<DeviceTreeNode> { }

    class TextHighLightPair
    {
        public int from;
        public int to;

        public TextHighLightPair(int From, int To)
        {
            from = From;
            to   = To;
        }
    }

   
    class TextHighLightPairs : List<TextHighLightPair> 
    { 
        public bool IsInPair(int indexOf)
        {
            bool bInPair = false;
            foreach(TextHighLightPair pair in this)
            {
                if(pair.from <= indexOf && pair.to >= indexOf)
                {
                    bInPair = true;
                    break;
                }
            }
            return bInPair;
        }
    }

    class DeviceVariableItem : ListViewItem
    {
        private ResultFileLib.Variable variable;
        public DeviceVariableItem(ResultFileLib.Variable Variable)
        {
            variable = Variable;
            Text = variable.Name;
            SubItems.Add(new ListViewSubItem(this, variable.UnitsName));
        }

        public ResultFileLib.Variable Variable
        {
            get { return variable; }
        }
    }

    class ModelVariableItem : ListViewItem
    {
        public ModelVariableItem()
        {
            Text = "Шаг";
            SubItems.Add(new ListViewSubItem(this, "c"));
        }
    }


    class DeviceTreeNode : TreeNode
    {
        private ResultFileLib.Device device;
        private DeviceTreeNodeList children;
        private DeviceTreeNodePool nodePool;
        TextHighLightPairs highLightPairs;

        private bool visibility;

        public DeviceTreeNode DeviceTreeParent
        {
            get;
            set;
        }

        public System.Drawing.Brush HighLightBrush
        {
            get { return nodePool.HighLightBrush; }
        }

        public System.Drawing.Brush WindowBrush
        {
            get { return nodePool.WindowBrush; }
        }

        public TextHighLightPairs HighLightPairs
        {
            get { return highLightPairs; }
        }
        
        public DeviceTreeNode(ResultFileLib.Device Device, DeviceTreeNodePool NodePool)
        {
            device = Device;
            nodePool = NodePool;
            children = new DeviceTreeNodeList();
            highLightPairs = new TextHighLightPairs();
            Text = string.Format("{0} {1} {2}", device.TypeName, device.Id, device.Name);
        }

        public string TypeName
        {
            get { return device.TypeName; }
        }

        public ResultFileLib.Devices ResultChildren
        {
            get { return device.Children;  }
        }

        public DeviceTreeNodeList Children
        {
            get { return children; }
        }

        public ResultFileLib.Variables Variables
        {
            get { return device.Variables; }
        }

        public DeviceTreeNode AddChild(ResultFileLib.Device Device)
        {
            DeviceTreeNode newDev = nodePool.NewDeviceTreeNode(Device);
            Nodes.Add(newDev);
            children.Add(newDev);
            newDev.DeviceTreeParent = this;
            return newDev;
        }

        public void Populate()
        {
            DeviceTreeNodeStack stack = new DeviceTreeNodeStack();
            stack.Push(this);
            while (stack.Count > 0)
            {
                DeviceTreeNode newNode = stack.Pop();
                foreach (ResultFileLib.Device dev in newNode.ResultChildren)
                {
                    if (dev.HasVariables || dev.Children.Count > 0)
                    {
                        DeviceTreeNode childNode = newNode.AddChild(dev);
                        stack.Push(childNode);
                    }
                }
            }
        }

        public bool Visible
        {
            get { return visibility; }
            set 
            { 
                visibility = value;  
                if(visibility)
                {
                    if (DeviceTreeParent != null)
                        DeviceTreeParent.Visible = value;
                }
            }
        }
        
        public void Hide()
        {
            if (Parent != null)
                Parent.Nodes.Remove(this);
        }

        public void Show()
        {
            if (DeviceTreeParent != null)
            {
                if (!DeviceTreeParent.Nodes.Contains(this))
                    DeviceTreeParent.Nodes.Add(this);
            }
        }

        public void TextFilter(string[] strArray, bool AndMode)
        {
            highLightPairs.Clear();

            if(strArray.Length == 0)
            {
                Visible = true;
                return;
            }

            Visible = false;

            string TextSearch = Text.ToUpper();

            if(AndMode)
            {
                bool eachRealEntryFound = true;
                foreach (string str in strArray)
                {
                    if (str.Length > 0)
                    {
                        int startIndex = 0;
                        bool bSearchAgain = true;

                        while (bSearchAgain)
                        {
                            int indexOf = TextSearch.IndexOf(str, startIndex);
                            if (indexOf >= 0)
                            {
                                if (highLightPairs.IsInPair(indexOf))
                                {
                                    startIndex = indexOf + 1;
                                    if (startIndex >= TextSearch.Length)
                                    {
                                        eachRealEntryFound = false;
                                        bSearchAgain = false;
                                        break;
                                    }
                                }
                                else
                                {
                                    highLightPairs.Add(new TextHighLightPair(indexOf, indexOf + str.Length));
                                    bSearchAgain = false;
                                    break;
                                }
                            }
                            else
                            {
                                bSearchAgain = false;
                                eachRealEntryFound = false;
                                break;
                            }
                        }
                    }
                }

                if (eachRealEntryFound)
                    Visible = true;
            }
            else
            {

                foreach (string str in strArray)
                {
                    if (str.Length > 0)
                    {
                        int startIndex = 0;
                        bool bSearchAgain = true;
                        while (bSearchAgain)
                        {
                            int indexOf = TextSearch.IndexOf(str, startIndex);
                            if (indexOf >= 0)
                            {
                                if (highLightPairs.IsInPair(indexOf))
                                {
                                    startIndex = indexOf + 1;
                                    if (startIndex >= TextSearch.Length)
                                    {
                                        bSearchAgain = false;
                                        break;
                                    }

                                }
                                else
                                {
                                    highLightPairs.Add(new TextHighLightPair(indexOf, indexOf + str.Length));
                                    bSearchAgain = false;
                                    break;
                                }
                            }
                            else
                            {
                                bSearchAgain = false;
                                break;
                            }
                        }
                    }
                }

                if (highLightPairs.Count > 0)
                    Visible = true;
            }
        }
    }

    class DeviceTreeNodePool
    {
        private DeviceTreeNodeList nodeList = new DeviceTreeNodeList();
        private System.Drawing.SolidBrush highlightBrush = new System.Drawing.SolidBrush(System.Drawing.SystemColors.Highlight);
        private System.Drawing.SolidBrush windowBrush = new System.Drawing.SolidBrush(System.Drawing.SystemColors.Window);

        public DeviceTreeNode NewDeviceTreeNode(ResultFileLib.Device Device)
        {
            DeviceTreeNode newNode = new DeviceTreeNode(Device, this);
            nodeList.Add(newNode);
            return newNode;
        }

        public DeviceTreeNodeList Pool
        {
            get { return nodeList; }
        }

        public System.Drawing.Brush HighLightBrush
        {
            get { return highlightBrush;  }
        }

        public System.Drawing.Brush WindowBrush
        {
            get { return windowBrush; }
        }

        public void FilterTree(string Filter, bool FilterMode)
        {
            foreach(DeviceTreeNode dev in nodeList)
            {
                dev.Visible = false;
            }

            string[] strArray = Filter.ToUpper().Split(' ');

            strArray = strArray.Where(val => val.Length > 0).ToArray();

            foreach (DeviceTreeNode dev in nodeList)
            {
                dev.TextFilter(strArray, FilterMode);
            }

            foreach(DeviceTreeNode dev in nodeList)
            {
                if (dev.Visible)
                    dev.Show();
                else
                    dev.Hide();
            }
        }
    }
}

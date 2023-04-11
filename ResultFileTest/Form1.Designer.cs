namespace ResultFileTest
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.ChartContainer = new System.Windows.Forms.SplitContainer();
            this.TreeAndVarsContainer = new System.Windows.Forms.SplitContainer();
            this.DeviceTree = new System.Windows.Forms.TreeView();
            this.VarBox = new System.Windows.Forms.ListView();
            this.menuStrip = new System.Windows.Forms.MenuStrip();
            this.FilterBox = new ResultFileTest.ButtonTextBox();
            ((System.ComponentModel.ISupportInitialize)(this.ChartContainer)).BeginInit();
            this.ChartContainer.Panel1.SuspendLayout();
            this.ChartContainer.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.TreeAndVarsContainer)).BeginInit();
            this.TreeAndVarsContainer.Panel1.SuspendLayout();
            this.TreeAndVarsContainer.Panel2.SuspendLayout();
            this.TreeAndVarsContainer.SuspendLayout();
            this.SuspendLayout();
            // 
            // ChartContainer
            // 
            this.ChartContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.ChartContainer.Location = new System.Drawing.Point(0, 24);
            this.ChartContainer.Name = "ChartContainer";
            // 
            // ChartContainer.Panel1
            // 
            this.ChartContainer.Panel1.Controls.Add(this.TreeAndVarsContainer);
            this.ChartContainer.Size = new System.Drawing.Size(621, 418);
            this.ChartContainer.SplitterDistance = 207;
            this.ChartContainer.TabIndex = 4;
            // 
            // TreeAndVarsContainer
            // 
            this.TreeAndVarsContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TreeAndVarsContainer.Location = new System.Drawing.Point(0, 0);
            this.TreeAndVarsContainer.Margin = new System.Windows.Forms.Padding(0);
            this.TreeAndVarsContainer.Name = "TreeAndVarsContainer";
            // 
            // TreeAndVarsContainer.Panel1
            // 
            this.TreeAndVarsContainer.Panel1.Controls.Add(this.DeviceTree);
            this.TreeAndVarsContainer.Panel1.Controls.Add(this.FilterBox);
            // 
            // TreeAndVarsContainer.Panel2
            // 
            this.TreeAndVarsContainer.Panel2.Controls.Add(this.VarBox);
            this.TreeAndVarsContainer.Size = new System.Drawing.Size(207, 418);
            this.TreeAndVarsContainer.SplitterDistance = 146;
            this.TreeAndVarsContainer.TabIndex = 3;
            // 
            // DeviceTree
            // 
            this.DeviceTree.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.DeviceTree.DrawMode = System.Windows.Forms.TreeViewDrawMode.OwnerDrawText;
            this.DeviceTree.Location = new System.Drawing.Point(0, 19);
            this.DeviceTree.Margin = new System.Windows.Forms.Padding(0);
            this.DeviceTree.Name = "DeviceTree";
            this.DeviceTree.Size = new System.Drawing.Size(147, 399);
            this.DeviceTree.TabIndex = 0;
            this.DeviceTree.DrawNode += new System.Windows.Forms.DrawTreeNodeEventHandler(this.OnDrawNode);
            this.DeviceTree.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.OnItemSelect);
            // 
            // VarBox
            // 
            this.VarBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.VarBox.HideSelection = false;
            this.VarBox.Location = new System.Drawing.Point(0, 0);
            this.VarBox.MultiSelect = false;
            this.VarBox.Name = "VarBox";
            this.VarBox.Size = new System.Drawing.Size(57, 418);
            this.VarBox.TabIndex = 2;
            this.VarBox.UseCompatibleStateImageBehavior = false;
            this.VarBox.View = System.Windows.Forms.View.Details;
            this.VarBox.ItemDrag += new System.Windows.Forms.ItemDragEventHandler(this.OnVariableDrag);
            // 
            // menuStrip
            // 
            this.menuStrip.TabIndex = 5;
            // 
            // FilterBox
            // 
            this.FilterBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.FilterBox.FilterMode = false;
            this.FilterBox.Location = new System.Drawing.Point(0, 0);
            this.FilterBox.Margin = new System.Windows.Forms.Padding(0);
            this.FilterBox.Name = "FilterBox";
            this.FilterBox.Size = new System.Drawing.Size(147, 20);
            this.FilterBox.TabIndex = 1;
            this.FilterBox.TextChanged += new System.EventHandler(this.OnTextChanged);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(621, 442);
            this.Controls.Add(this.ChartContainer);
            this.Controls.Add(this.menuStrip);
            this.MainMenuStrip = this.menuStrip;
            this.Name = "Form1";
            this.Text = "Form1";
            this.Load += new System.EventHandler(this.OnLoad);
            this.ChartContainer.Panel1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.ChartContainer)).EndInit();
            this.ChartContainer.ResumeLayout(false);
            this.TreeAndVarsContainer.Panel1.ResumeLayout(false);
            this.TreeAndVarsContainer.Panel1.PerformLayout();
            this.TreeAndVarsContainer.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.TreeAndVarsContainer)).EndInit();
            this.TreeAndVarsContainer.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TreeView DeviceTree;
        private ButtonTextBox FilterBox;
        private System.Windows.Forms.ListView VarBox;
        private System.Windows.Forms.SplitContainer TreeAndVarsContainer;
        private System.Windows.Forms.SplitContainer ChartContainer;
        private System.Windows.Forms.MenuStrip menuStrip;
    }
}


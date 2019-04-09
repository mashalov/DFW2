using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Drawing;

namespace ResultFileTest
{

    class NoSelectButton : Button
    {
        public NoSelectButton()
        {
            SetStyle(ControlStyles.Selectable, false);
        }
    }

    public class ButtonTextBox : TextBox
    {
        private readonly NoSelectButton _buttonClear;
        private readonly NoSelectButton _buttonChangeFilter;
        private bool andFilterMode;

        public bool FilterMode
        {
            get { return andFilterMode; }
            set 
            { 
                andFilterMode = value;
                ShowFilterMode();
            }

        }

        //public event EventHandler FilterModeChanged { add { _button.Click += value; } remove { _button.Click -= value; } }

        private void ShowFilterMode()
        {
            _buttonChangeFilter.Image = andFilterMode ? Images.ButtonFilterTreeAnd : Images.ButtonFilterTreeOr;
        }

        private void ButtonClearClick(object sender, System.EventArgs e)
        {
            Text = "";
            Parent.Focus();
        }

        private void ButtonChangeFilterClick(object sender, System.EventArgs e)
        {
            FilterMode = !FilterMode;
            ShowFilterMode();
            Parent.Focus();
            OnTextChanged(new System.EventArgs());
        }


        public ButtonTextBox()
        {
            _buttonClear        = new NoSelectButton { Cursor = Cursors.Default };
            _buttonChangeFilter = new NoSelectButton { Cursor = Cursors.Default };

            _buttonClear.SizeChanged += (o, e) => OnResize(e);
            _buttonChangeFilter.SizeChanged += (o, e) => OnResize(e);

            _buttonClear.Click += ButtonClearClick;
            _buttonChangeFilter.Click += ButtonChangeFilterClick;

            _buttonClear.Image = Images.ButtonClearSmall;
            _buttonClear.BackColor = SystemColors.ButtonFace;
            _buttonChangeFilter.BackColor = SystemColors.ButtonFace;

            this.Controls.Add(_buttonClear);
            this.Controls.Add(_buttonChangeFilter);

            ShowFilterMode();
        }

        protected override void OnResize(EventArgs e)
        {
            base.OnResize(e);
            Size buttonSize = new Size(this.ClientSize.Height + 2, this.ClientSize.Height + 2);
            _buttonClear.Size = buttonSize;
            _buttonClear.Location = new Point(this.ClientSize.Width - _buttonClear.Width, -1);

            _buttonChangeFilter.Size = buttonSize;
            _buttonChangeFilter.Location = new Point(this.ClientSize.Width - _buttonClear.Width - _buttonChangeFilter.Width, -1);

            // Send EM_SETMARGINS to prevent text from disappearing underneath the button
            SendMessage(this.Handle, 0xd3, (IntPtr)2, (IntPtr)(_buttonClear.Width << 16));
        }

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wp, IntPtr lp);

    }
}

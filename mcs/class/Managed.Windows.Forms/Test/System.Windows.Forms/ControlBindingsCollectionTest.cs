// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Copyright (c) 2007 Novell, Inc.
//
// Authors:
//	Chris Toshok	toshok@ximian.com

using System;
using System.Data;
using System.Collections;
using System.Windows.Forms;

using NUnit.Framework;

namespace MonoTests.System.Windows.Forms.DataBinding {

	[TestFixture]
	public class ControlBindingsCollectionTest {

		[Test]
		[ExpectedException (typeof (ArgumentException))] // MS: "This would cause two bindings in the collection to bind to the same property."
		public void DuplicateBindingAdd ()
		{
			Control c1 = new Control ();
			Control c2 = new Control ();

			c2.DataBindings.Add ("Text", c1, "Text");
			c2.DataBindings.Add ("Text", c1, "Text");
		}

#if NET_2_0
		[Test]
		public void CtorTest ()
		{
			BindableToolStripItem item = new BindableToolStripItem ();
			ControlBindingsCollection data_bindings = new ControlBindingsCollection (item);

			Assert.AreEqual (item, data_bindings.BindableComponent, "#A1");
			Assert.AreEqual (null, data_bindings.Control, "#A2");
		}
#endif
	}

}

# test_file.py -- Test for git files
# Copyright (C) 2010 Google, Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License or (at your option) a later version of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA  02110-1301, USA.


import errno
import os
import shutil
import tempfile
import unittest

from dulwich.file import GitFile

class GitFileTests(unittest.TestCase):
    def setUp(self):
        self._tempdir = tempfile.mkdtemp()
        f = open(self.path('foo'), 'wb')
        f.write('foo contents')
        f.close()

    def tearDown(self):
        shutil.rmtree(self._tempdir)

    def path(self, filename):
        return os.path.join(self._tempdir, filename)

    def test_invalid(self):
        foo = self.path('foo')
        self.assertRaises(IOError, GitFile, foo, mode='r')
        self.assertRaises(IOError, GitFile, foo, mode='ab')
        self.assertRaises(IOError, GitFile, foo, mode='r+b')
        self.assertRaises(IOError, GitFile, foo, mode='w+b')
        self.assertRaises(IOError, GitFile, foo, mode='a+bU')

    def test_readonly(self):
        f = GitFile(self.path('foo'), 'rb')
        self.assertTrue(isinstance(f, file))
        self.assertEquals('foo contents', f.read())
        self.assertEquals('', f.read())
        f.seek(4)
        self.assertEquals('contents', f.read())
        f.close()

    def test_write(self):
        foo = self.path('foo')
        foo_lock = '%s.lock' % foo

        orig_f = open(foo, 'rb')
        self.assertEquals(orig_f.read(), 'foo contents')
        orig_f.close()

        self.assertFalse(os.path.exists(foo_lock))
        f = GitFile(foo, 'wb')
        self.assertFalse(f.closed)
        self.assertRaises(AttributeError, getattr, f, 'not_a_file_property')

        self.assertTrue(os.path.exists(foo_lock))
        f.write('new stuff')
        f.seek(4)
        f.write('contents')
        f.close()
        self.assertFalse(os.path.exists(foo_lock))

        new_f = open(foo, 'rb')
        self.assertEquals('new contents', new_f.read())
        new_f.close()

    def test_open_twice(self):
        foo = self.path('foo')
        f1 = GitFile(foo, 'wb')
        f1.write('new')
        try:
            f2 = GitFile(foo, 'wb')
            fail()
        except OSError, e:
            self.assertEquals(errno.EEXIST, e.errno)
        f1.write(' contents')
        f1.close()

        # Ensure trying to open twice doesn't affect original.
        f = open(foo, 'rb')
        self.assertEquals('new contents', f.read())
        f.close()

    def test_abort(self):
        foo = self.path('foo')
        foo_lock = '%s.lock' % foo

        orig_f = open(foo, 'rb')
        self.assertEquals(orig_f.read(), 'foo contents')
        orig_f.close()

        f = GitFile(foo, 'wb')
        f.write('new contents')
        f.abort()
        self.assertTrue(f.closed)
        self.assertFalse(os.path.exists(foo_lock))

        new_orig_f = open(foo, 'rb')
        self.assertEquals(new_orig_f.read(), 'foo contents')
        new_orig_f.close()

    def test_abort_close(self):
        foo = self.path('foo')
        f = GitFile(foo, 'wb')
        f.abort()
        try:
            f.close()
        except (IOError, OSError):
            self.fail()

        f = GitFile(foo, 'wb')
        f.close()
        try:
            f.abort()
        except (IOError, OSError):
            self.fail()

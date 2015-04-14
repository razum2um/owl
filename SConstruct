# -*- python -*-
#
# Owl PDF viewer
#
# Copyright (c) 2009 Zoltan Kovacs, Peter Szilagyi
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of version 2 of the GNU General Public License
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

import os

VariantDir( 'objs', 'src', duplicate = 0 )

# Command line parameters

vars = Variables( "build-setup.conf" )

vars.Add( BoolVariable( "RELEASE", "Set to 'true' to create a release build", False ) )
vars.Add( PathVariable( "PREFIX", "Set to the directory where Owl should be installed", "/usr", PathVariable.PathAccept ) )

envToSave = Environment( variables = vars )

vars.Add( PathVariable( "PKGPREFIX", "Set to the directory where the package is generated", "", PathVariable.PathAccept ) )

env = Environment( variables = vars )

if not os.path.isabs( env[ "PREFIX" ] ) :
    print "PREFIX must be absolute path"
    Exit( 1 )

if env[ "PKGPREFIX" ] != "" and not os.path.isabs( env[ "PKGPREFIX" ] ) :
    print "PKGPREFIX must be absolute path"
    Exit( 1 )

Help( vars.GenerateHelpText( env ) )

vars.Save( "build-setup.conf", envToSave )

# Compile part

env.ParseConfig( 'pkg-config --cflags --libs poppler-glib gthread-2.0 gtk+-2.0' )
env.Append( CPPPATH = 'objs' )

if env[ "RELEASE" ] :
    print "Release build"
    env[ 'CPPFLAGS' ] = '-g -O2 -Wall'
else :
    print "Debug build"
    env[ 'CPPFLAGS' ] = '-g -ggdb -O0 -rdynamic -Wall'

env[ 'CPPDEFINES' ] = '-DOWL_INSTALL_PATH="\\"' + env[ "PREFIX" ] + '/share/owl' +  '\\""'

env[ 'CCCOMSTR' ] = 'Compiling $SOURCE'
env[ 'LINKCOMSTR' ] = 'Linking $TARGET'

env.Program(
    target = 'owl',
    source = [
        Glob( 'objs/*.c' ),
        Glob( 'objs/engine/*.c' ),
        Glob( 'objs/gui/*.c' ),
        Glob( 'objs/config/*.c' ),
        Glob( 'objs/mgmt/*.c' )
    ]
)

# Install part

data = env.Install( env[ "PKGPREFIX" ] + env[ "PREFIX" ] + '/bin', [ "owl" ] )
env.Alias( "install", data )

data = env.Install( env[ "PKGPREFIX" ] + env[ "PREFIX" ] + '/share/owl/pixmaps/', Glob( "pixmaps/*.png" ) )
env.Alias( "install", data )

# Uninstall part

if 'uninstall' in COMMAND_LINE_TARGETS :
    env.Command( "uninstall-bin", "", [ Delete( env[ "PREFIX" ] + "/bin/owl" ) ] )
    env.Command( "uninstall-data", "", [ Delete( env[ "PREFIX" ] + "/share/owl" ) ] )
    env.Alias( "uninstall", "uninstall-bin" )
    env.Alias( "uninstall", "uninstall-data" )

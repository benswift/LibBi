=head1 NAME

Bi::Builder - builds client programs on demand.

=head1 SYNOPSIS

    use Bi::Builder
    my $builder = new Bi::Builder($dir);
    my $client = 'simulate';  # or 'filter' or 'smooth' etc
    $builder->build($client);

=head1 COMMAND LINE

Options are read from the command line to control various aspects of the
build. See the Bi user manual for details.

=head1 METHODS

=over 4

=cut

package Bi::Builder;

use warnings;
use strict;

use Carp::Assert;
use Cwd qw(abs_path getcwd);
use Getopt::Long qw(:config pass_through);
use File::Spec;
use File::Slurp;

=item B<new>(I<builddir>, I<verbose>)

Constructor.

=over 4

=item * I<builddir> The build directory

=item * I<verbose> Is verbosity enabled?

=back

Returns the new object.

=cut
sub new {
    my $class = shift;
    my $builddir = shift;
    my $verbose = int(shift);
    
    $builddir = abs_path($builddir);
    my $self = {
        _builddir => $builddir,
        _verbose => $verbose,
        _warn => 0,
        _native => 0,
        _force => 0,
        _assert => 1,
        _cuda => 0,
        _sse => 0,
	_mkl => 0,
        _mpi => 0,
        _vampir => 0,
        _single => 0,
        _tstamps => {}
    };
    bless $self, $class;
    
    # command line options
    my @args = (
        'warn' => \$self->{_warn},
        'native' => \$self->{_native},
        'force' => \$self->{_force},
        'enable-assert' => sub { $self->{_assert} = 1 },
        'disable-assert' => sub { $self->{_assert} = 0 },
        'enable-cuda' => sub { $self->{_cuda} = 1 },
        'disable-cuda' => sub { $self->{_cuda} = 0 },
        'enable-sse' => sub { $self->{_sse} = 1 },
        'disable-sse' => sub { $self->{_sse} = 0 },
        'enable-mkl' => sub { $self->{_mkl} = 1 },
        'disable-mkl' => sub { $self->{_mkl} = 0 },
        'enable-mpi' => sub { $self->{_mpi} = 1 },
        'disable-mpi' => sub { $self->{_mpi} = 0 },
        'enable-vampir' => sub { $self->{_vampir} = 1 },
        'disable-vampir' => sub { $self->{_vampir} = 0 },
        'enable-single' => sub { $self->{_single} = 1 },
        'disable-single' => sub { $self->{_single} = 0 },
    );
    GetOptions(@args) || die("could not read command line arguments\n");
    
    # time stamp dependencies
    $self->_stamp(File::Spec->catfile($builddir, 'src', 'options.hpp'));
    $self->_stamp(File::Spec->catfile($builddir, 'autogen.sh'));
    $self->_stamp(File::Spec->catfile($builddir, 'configure.ac'));
    $self->_stamp(File::Spec->catfile($builddir, 'Makefile.am'));
    $self->_stamp(File::Spec->catfile($builddir, 'configure'));
    $self->_stamp(File::Spec->catfile($builddir, 'Makefile'));
    
    return $self;
}

=item B<build>(I<client>)

Build a client program.

=over 4

=item I<client> The name of the client program ('simulate', 'filter',
'pmcmc', etc).

=back

No return value.

=cut
sub build {
    my $self = shift;
    my $client = shift;
    
    $self->_autogen;
    $self->_configure;
    $self->_make($client);
}

=item B<_autogen>

Run the C<autogen.sh> script if one or more of it, C<configure.ac> or
C<Makefile.am> has been modified since the last run.

No return value.

=cut
sub _autogen {
    my $self = shift;
    my $builddir = $self->{_builddir};
        
    if ($self->{_force} ||
        $self->_is_modified(File::Spec->catfile($builddir, 'autogen.sh')) ||
        $self->_is_modified(File::Spec->catfile($builddir, 'configure.ac')) ||
        $self->_is_modified(File::Spec->catfile($builddir, 'Makefile.am'))) {          
        my $cwd = getcwd();
        
        chdir($builddir) || die("could not change to build directory '$builddir'\n");
        my $ret = system('./autogen.sh');
        if ($? == -1) {
            die("./autogen.sh failed to execute ($!)\n");
        } elsif ($? & 127) {
            die(sprintf("./autogen.sh died with signal %d\n", $? & 127));
        } elsif ($ret != 0) {
            die(sprintf("./autogen.sh failed with return code %d\n", $ret >> 8));
        }
        
        chdir($cwd) || warn("could not change back to working directory '$cwd'\n");
    }
}

=item B<_configure>

Run the C<configure> script if it has been modified since the last run.

No return value.

=cut
sub _configure {
    my $self = shift;

    my $builddir = $self->{_builddir};
    my $cwd = getcwd();
    my $cxxflags = '-O3 -g3 -funroll-loops';
    my $linkflags = '';
    my $options = '';

    if (!$self->{_verbose}) {
        $options .= ' --silent'; 
    }
    if (!$self->{_force}) {
        $options .= ' --config-cache';
    }

    $options .= $self->{_assert} ? ' --enable-assert' : ' --disable-assert';
    $options .= $self->{_cuda} ? ' --enable-cuda' : ' --disable-cuda';
    $options .= $self->{_sse} ? ' --enable-sse' : ' --disable-sse';
    $options .= $self->{_mkl} ? ' --enable-mkl' : ' --disable-mkl';
    $options .= $self->{_mpi} ? ' --enable-mpi' : ' --disable-mpi';
    $options .= $self->{_vampir} ? ' --enable-vampir' : ' --disable-vampir';
    $options .= $self->{_single} ? ' --enable-single' : ' --disable-single';
    
    # write options file
    my $options_file = File::Spec->catfile($builddir, 'src', 'options.hpp');
    my $options_content = "//$options\n";
        
    if (!-e $options_file || read_file($options_file) ne $options_content) {
    	write_file($options_file, $options_content);
    }
        
    if ($self->{_warn}) {
        $cxxflags .= " -Wall";
        $linkflags .= " -Wall";
    }
    if ($self->{_native}) {
        $cxxflags .= " -march=native";
    }

    if ($self->{_force} ||
        $self->_is_modified($options_file) ||
        $self->_is_modified(File::Spec->catfile($builddir, 'configure'))) {
        
        my $cmd = "./configure $options CXXFLAGS='$cxxflags' LINKFLAGS='$linkflags'";
        if ($self->{_verbose}) {
            print "$cmd\n";
        }
        
        chdir($builddir);
        my $ret = system($cmd);
        if ($? == -1) {
            die("./configure failed to execute ($!)\n");
        } elsif ($? & 127) {
            die(sprintf("./configure died with signal %d\n", $? & 127));
        } elsif ($ret != 0) {
            die(sprintf("./configure failed with return code %d\n", $ret >> 8));
        }        
        chdir($cwd);
    }
}

=item B<_make>(I<client>)

Run C<make> to compile the given client program.

=over 4

=item I<client> The name of the client program ('simulate', 'filter',
'pmcmc', etc).

=back

No return value.

=cut
sub _make {
    my $self = shift;
    my $client = shift;
    
    my $target = $client . "_" . ($self->{_cuda} ? 'gpu' : 'cpu');
    my $options = '';
    if (!$self->{_verbose}) {
        $options .= ' --quiet'; 
    }
    if ($self->{_force}) {
        $options .= ' --always-make';
    }
    
    my $builddir = $self->{_builddir};
    my $cwd = getcwd();
    my $cmd = "make -j 4 $options $target";
    
    if ($self->{_verbose}) {
        print "$cmd\n";
    }
    
    chdir($builddir);
    my $ret = system($cmd);
    if ($? == -1) {
        die("make failed to execute ($!)\n");
    } elsif ($? & 127) {
        die(sprintf("make died with signal %d\n", $? & 127));
    } elsif ($ret != 0) {
        die(sprintf("make failed with return code %d\n", $ret >> 8));
    }
    symlink($target, $client);
    chdir($cwd);
}

=item B<_stamp>(I<filename>)

Update timestamp on file.

=over 4

=item I<filename> The name of the file.

=back

No return value.

=cut
sub _stamp {
    my $self = shift;
    my $filename = shift;

    $self->{_tstamps}->{$filename} = _last_modified($filename);
}

=item B<_is_modified>(I<filename>)

Has file been modified since last use?

=over 4

=item I<filename> The name of the file.

=back

Returns true if the file has been modified since last use.

=cut
sub _is_modified {
    my $self = shift;
    my $filename = shift;

    return (!exists $self->{_tstamps}->{$filename} ||
        _last_modified($filename) < $self->{_tstamps}->{$filename});
}

=item B<_last_modified>(I<filename>)

Get time of last modification of a file.

=over 4

=item I<filename> The name of the file.

=back

Returns the time of last modification of the file, or the current time if
the file does not exist.

=cut
sub _last_modified {
    my $filename = shift;
    
    if (-e $filename) {
        return abs(-M $filename); # modified time in stat() has insufficient res
    } else {
        return time;
    }
}

1;

=back

=head1 AUTHOR

Lawrence Murray <lawrence.murray@csiro.au>

=head1 VERSION

$Rev$ $Date$
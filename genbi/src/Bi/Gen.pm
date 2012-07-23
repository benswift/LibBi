=head1 NAME

Bi::Gen - generic code generator for model specification.

=head1 SYNOPSIS

Use subclass.

=head1 REQUIRES

L<Template>

=head1 METHODS

=over 4

=cut

package Bi::Gen;

use warnings;
use strict;

use Carp::Assert;
use Template;
use Template::Filters;
use Template::Exception;
use IO::File;
use File::Compare;
use File::Copy;
use File::Path;
use File::Spec;
use File::Slurp;

use Bi::Model;
use Bi::Expression;
use Bi::Visitor::ToCpp;

=item B<new>(I<ttdirs>, I<outdir>)

Constructor.

=over 4

=item I<ttdirs> 

Array reference, of directories containing Perl Template
Toolkit templates, or scalar giving single directory.

=item I<outdir> (optional)

Directory in which to output results.

=back

Returns the new object.

=cut
sub new {
    my $class = shift;
    my $ttdirs = shift;
    my $outdir = shift;

    if (ref($ttdirs) ne 'ARRAY') {
        $ttdirs = [ $ttdirs ];
    }
    my $tt = Template->new({
        INCLUDE_PATH => $ttdirs,
        FILTERS => {},
        RECURSION => 1,
        STRICT => 1
    }) || _error($Template::ERROR);
        
    my $self = {
        _tt => $tt,
        _ttdirs => $ttdirs,
        _outdir => $outdir
    };
    bless $self, $class;
    
    return $self;
}

=item B<get_tt>

Get Perl template toolkit object.

=cut
sub get_tt {
    my $self = shift;
    return $self->{_tt};
}

=item B<is_template>(I<filename>)

Does a template of the given I<filename> exist?

=cut
sub is_template {
    my $self = shift;
    my $filename = shift;
    
    my $ttdir;
    foreach $ttdir (@{$self->{_ttdirs}}) {
        if (-e "$ttdir/$filename") {
            return 1;
        }        
    }
    return 0;
}

=item B<process_template>(I<template>, I<output>, I<vars>)

Process template.

=over 4

=item I<template> Template file name.

=item I<output> Output file name, or C<undef> for no output.

=item I<vars> Hash to be passed to template processor.

=back

No return value.

=cut
sub process_template {
    my $self = shift;
    my $template = shift;
    my $vars = shift;
    my $output = shift;
    
    my $null = undef;
    my $tt = $self->{_tt};
    my $to_file = defined($output) && ref($output) ne 'GLOB';
    my $out;
    if ($to_file) {
        $out = File::Spec->catfile($self->{_outdir}, $output);
    } elsif (!ref($output) eq 'GLOB') {
        $out = \$null;
    }
    my ($vol, $dir, $file);
    
    # create build directory
    mkpath($self->{_outdir});
    
    if ($to_file) {
        # write to temp file first, only copy over desired output if file
        # contents changes; keeps last modified dates, used by make,
        # consistent with actual file changes
        my $str;
        $tt->process($template, $vars, \$str) || _error($tt->error());
        if (!-e $out || read_file($out) ne $str) {
            ($vol, $dir, $file) = File::Spec->splitpath($out);
            mkpath($dir);
            write_file($out, $str);
        }
        if ($output =~ /\.sh$/) {
            chmod(0755, $out);
        }
    } else {
        $tt->process($template, $vars, $out) || _error($tt->error());
    }
}

=back

=head1 CLASS METHODS

=item B<_error>(I<msg>)

Print I<msg> as error.

=cut
sub _error {
    my $msg = shift;

    # Perl Template Tookit puts exception name at start of error string,
    # remove this if present
    $msg =~ s/.*? - //;

    die("$msg\n");
}

1;

=back

=head1 AUTHOR

Lawrence Murray <lawrence.murray@csiro.au>

=head1 VERSION

$Rev$ $Date$
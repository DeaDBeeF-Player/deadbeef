#       $Id$

package File::Find::Rule;
use strict;
use File::Spec;
use Text::Glob 'glob_to_regex';
use Number::Compare;
use Carp qw/croak/;
use File::Find (); # we're only wrapping for now

our $VERSION = '0.33';

# we'd just inherit from Exporter, but I want the colon
sub import {
    my $pkg = shift;
    my $to  = caller;
    for my $sym ( qw( find rule ) ) {
        no strict 'refs';
        *{"$to\::$sym"} = \&{$sym};
    }
    for (grep /^:/, @_) {
        my ($extension) = /^:(.*)/;
        eval "require File::Find::Rule::$extension";
        croak "couldn't bootstrap File::Find::Rule::$extension: $@" if $@;
    }
}

=head1 NAME

File::Find::Rule - Alternative interface to File::Find

=head1 SYNOPSIS

  use File::Find::Rule;
  # find all the subdirectories of a given directory
  my @subdirs = File::Find::Rule->directory->in( $directory );

  # find all the .pm files in @INC
  my @files = File::Find::Rule->file()
                              ->name( '*.pm' )
                              ->in( @INC );

  # as above, but without method chaining
  my $rule =  File::Find::Rule->new;
  $rule->file;
  $rule->name( '*.pm' );
  my @files = $rule->in( @INC );

=head1 DESCRIPTION

File::Find::Rule is a friendlier interface to File::Find.  It allows
you to build rules which specify the desired files and directories.

=cut

# the procedural shim

*rule = \&find;
sub find {
    my $object = __PACKAGE__->new();
    my $not = 0;

    while (@_) {
        my $method = shift;
        my @args;

        if ($method =~ s/^\!//) {
            # jinkies, we're really negating this
            unshift @_, $method;
            $not = 1;
            next;
        }
        unless (defined prototype $method) {
            my $args = shift;
            @args = ref $args eq 'ARRAY' ? @$args : $args;
        }
        if ($not) {
            $not = 0;
            @args = $object->new->$method(@args);
            $method = "not";
        }

        my @return = $object->$method(@args);
        return @return if $method eq 'in';
    }
    $object;
}


=head1 METHODS

=over

=item C<new>

A constructor.  You need not invoke C<new> manually unless you wish
to, as each of the rule-making methods will auto-create a suitable
object if called as class methods.

=cut

sub new {
    my $referent = shift;
    my $class = ref $referent || $referent;
    bless {
        rules    => [],
        subs     => {},
        iterator => [],
        extras   => {},
        maxdepth => undef,
        mindepth => undef,
    }, $class;
}

sub _force_object {
    my $object = shift;
    $object = $object->new()
      unless ref $object;
    $object;
}

=back

=head2 Matching Rules

=over

=item C<name( @patterns )>

Specifies names that should match.  May be globs or regular
expressions.

 $set->name( '*.mp3', '*.ogg' ); # mp3s or oggs
 $set->name( qr/\.(mp3|ogg)$/ ); # the same as a regex
 $set->name( 'foo.bar' );        # just things named foo.bar

=cut

sub _flatten {
    my @flat;
    while (@_) {
        my $item = shift;
        ref $item eq 'ARRAY' ? push @_, @{ $item } : push @flat, $item;
    }
    return @flat;
}

sub name {
    my $self = _force_object shift;
    my @names = map { ref $_ eq "Regexp" ? $_ : glob_to_regex $_ } _flatten( @_ );

    push @{ $self->{rules} }, {
        rule => 'name',
        code => join( ' || ', map { "m{$_}" } @names ),
        args => \@_,
    };

    $self;
}

=item -X tests

Synonyms are provided for each of the -X tests. See L<perlfunc/-X> for
details.  None of these methods take arguments.

  Test | Method               Test |  Method
 ------|-------------        ------|----------------
   -r  |  readable             -R  |  r_readable
   -w  |  writeable            -W  |  r_writeable
   -w  |  writable             -W  |  r_writable
   -x  |  executable           -X  |  r_executable
   -o  |  owned                -O  |  r_owned
       |                           |
   -e  |  exists               -f  |  file
   -z  |  empty                -d  |  directory
   -s  |  nonempty             -l  |  symlink
       |                       -p  |  fifo
   -u  |  setuid               -S  |  socket
   -g  |  setgid               -b  |  block
   -k  |  sticky               -c  |  character
       |                       -t  |  tty
   -M  |  modified                 |
   -A  |  accessed             -T  |  ascii
   -C  |  changed              -B  |  binary

Though some tests are fairly meaningless as binary flags (C<modified>,
C<accessed>, C<changed>), they have been included for completeness.

 # find nonempty files
 $rule->file,
      ->nonempty;

=cut

use vars qw( %X_tests );
%X_tests = (
    -r  =>  readable           =>  -R  =>  r_readable      =>
    -w  =>  writeable          =>  -W  =>  r_writeable     =>
    -w  =>  writable           =>  -W  =>  r_writable      =>
    -x  =>  executable         =>  -X  =>  r_executable    =>
    -o  =>  owned              =>  -O  =>  r_owned         =>

    -e  =>  exists             =>  -f  =>  file            =>
    -z  =>  empty              =>  -d  =>  directory       =>
    -s  =>  nonempty           =>  -l  =>  symlink         =>
                               =>  -p  =>  fifo            =>
    -u  =>  setuid             =>  -S  =>  socket          =>
    -g  =>  setgid             =>  -b  =>  block           =>
    -k  =>  sticky             =>  -c  =>  character       =>
                               =>  -t  =>  tty             =>
    -M  =>  modified                                       =>
    -A  =>  accessed           =>  -T  =>  ascii           =>
    -C  =>  changed            =>  -B  =>  binary          =>
   );

for my $test (keys %X_tests) {
    my $sub = eval 'sub () {
        my $self = _force_object shift;
        push @{ $self->{rules} }, {
            code => "' . $test . ' \$_",
            rule => "'.$X_tests{$test}.'",
        };
        $self;
    } ';
    no strict 'refs';
    *{ $X_tests{$test} } = $sub;
}


=item stat tests

The following C<stat> based methods are provided: C<dev>, C<ino>,
C<mode>, C<nlink>, C<uid>, C<gid>, C<rdev>, C<size>, C<atime>,
C<mtime>, C<ctime>, C<blksize>, and C<blocks>.  See L<perlfunc/stat>
for details.

Each of these can take a number of targets, which will follow
L<Number::Compare> semantics.

 $rule->size( 7 );         # exactly 7
 $rule->size( ">7Ki" );    # larger than 7 * 1024 * 1024 bytes
 $rule->size( ">=7" )
      ->size( "<=90" );    # between 7 and 90, inclusive
 $rule->size( 7, 9, 42 );  # 7, 9 or 42

=cut

use vars qw( @stat_tests );
@stat_tests = qw( dev ino mode nlink uid gid rdev
                  size atime mtime ctime blksize blocks );
{
    my $i = 0;
    for my $test (@stat_tests) {
        my $index = $i++; # to close over
        my $sub = sub {
            my $self = _force_object shift;

            my @tests = map { Number::Compare->parse_to_perl($_) } @_;

            push @{ $self->{rules} }, {
                rule => $test,
                args => \@_,
                code => 'do { my $val = (stat $_)['.$index.'] || 0;'.
                  join ('||', map { "(\$val $_)" } @tests ).' }',
            };
            $self;
        };
        no strict 'refs';
        *$test = $sub;
    }
}

=item C<any( @rules )>

=item C<or( @rules )>

Allows shortcircuiting boolean evaluation as an alternative to the
default and-like nature of combined rules.  C<any> and C<or> are
interchangeable.

 # find avis, movs, things over 200M and empty files
 $rule->any( File::Find::Rule->name( '*.avi', '*.mov' ),
             File::Find::Rule->size( '>200M' ),
             File::Find::Rule->file->empty,
           );

=cut

sub any {
    my $self = _force_object shift;
    # compile all the subrules to code fragments
    push @{ $self->{rules} }, {
        rule => "any",
        code => '(' . join( ' || ', map '( ' . $_->_compile . ' )', @_ ). ')',
        args => \@_,
    };
    
    # merge all the subs hashes of the kids into ourself
    %{ $self->{subs} } = map { %{ $_->{subs} } } $self, @_;
    $self;
}

*or = \&any;

=item C<none( @rules )>

=item C<not( @rules )>

Negates a rule.  (The inverse of C<any>.)  C<none> and C<not> are
interchangeable.

  # files that aren't 8.3 safe
  $rule->file
       ->not( $rule->new->name( qr/^[^.]{1,8}(\.[^.]{0,3})?$/ ) );

=cut

sub not {
    my $self = _force_object shift;

    push @{ $self->{rules} }, {
        rule => 'not',
        args => \@_,
        code => '(' . join ( ' && ', map { "!(". $_->_compile . ")" } @_ ) . ")",
    };
    
    # merge all the subs hashes into us
    %{ $self->{subs} } = map { %{ $_->{subs} } } $self, @_;
    $self;
}

*none = \&not;

=item C<prune>

Traverse no further.  This rule always matches.

=cut

sub prune () {
    my $self = _force_object shift;

    push @{ $self->{rules} },
      {
       rule => 'prune',
       code => '$File::Find::prune = 1'
      };
    $self;
}

=item C<discard>

Don't keep this file.  This rule always matches.

=cut

sub discard () {
    my $self = _force_object shift;

    push @{ $self->{rules} }, {
        rule => 'discard',
        code => '$discarded = 1',
    };
    $self;
}

=item C<exec( \&subroutine( $shortname, $path, $fullname ) )>

Allows user-defined rules.  Your subroutine will be invoked with C<$_>
set to the current short name, and with parameters of the name, the
path you're in, and the full relative filename.

Return a true value if your rule matched.

 # get things with long names
 $rules->exec( sub { length > 20 } );

=cut

sub exec {
    my $self = _force_object shift;
    my $code = shift;

    push @{ $self->{rules} }, {
        rule => 'exec',
        code => $code,
    };
    $self;
}

=item C<grep( @specifiers )>

Opens a file and tests it each line at a time.

For each line it evaluates each of the specifiers, stopping at the
first successful match.  A specifier may be a regular expression or a
subroutine.  The subroutine will be invoked with the same parameters
as an ->exec subroutine.

It is possible to provide a set of negative specifiers by enclosing
them in anonymous arrays.  Should a negative specifier match the
iteration is aborted and the clause is failed.  For example:

 $rule->grep( qr/^#!.*\bperl/, [ sub { 1 } ] );

Is a passing clause if the first line of a file looks like a perl
shebang line.

=cut

sub grep {
    my $self = _force_object shift;
    my @pattern = map {
        ref $_
          ? ref $_ eq 'ARRAY'
            ? map { [ ( ref $_ ? $_ : qr/$_/ ) => 0 ] } @$_
            : [ $_ => 1 ]
          : [ qr/$_/ => 1 ]
      } @_;

    $self->exec( sub {
        local *FILE;
        open FILE, $_ or return;
        local ($_, $.);
        while (<FILE>) {
            for my $p (@pattern) {
                my ($rule, $ret) = @$p;
                return $ret
                  if ref $rule eq 'Regexp'
                    ? /$rule/
                      : $rule->(@_);
            }
        }
        return;
    } );
}

=item C<maxdepth( $level )>

Descend at most C<$level> (a non-negative integer) levels of directories
below the starting point.

May be invoked many times per rule, but only the most recent value is
used.

=item C<mindepth( $level )>

Do not apply any tests at levels less than C<$level> (a non-negative
integer).

=item C<extras( \%extras )>

Specifies extra values to pass through to C<File::File::find> as part
of the options hash.

For example this allows you to specify following of symlinks like so:

 my $rule = File::Find::Rule->extras({ follow => 1 });

May be invoked many times per rule, but only the most recent value is
used.

=cut

for my $setter (qw( maxdepth mindepth extras )) {
    my $sub = sub {
        my $self = _force_object shift;
        $self->{$setter} = shift;
        $self;
    };
    no strict 'refs';
    *$setter = $sub;
}


=item C<relative>

Trim the leading portion of any path found

=cut

sub relative () {
    my $self = _force_object shift;
    $self->{relative} = 1;
    $self;
}

=item C<not_*>

Negated version of the rule.  An effective shortand related to ! in
the procedural interface.

 $foo->not_name('*.pl');

 $foo->not( $foo->new->name('*.pl' ) );

=cut

sub DESTROY {}
sub AUTOLOAD {
    our $AUTOLOAD;
    $AUTOLOAD =~ /::not_([^:]*)$/
      or croak "Can't locate method $AUTOLOAD";
    my $method = $1;

    my $sub = sub {
        my $self = _force_object shift;
        $self->not( $self->new->$method(@_) );
    };
    {
        no strict 'refs';
        *$AUTOLOAD = $sub;
    }
    &$sub;
}

=back

=head2 Query Methods

=over

=item C<in( @directories )>

Evaluates the rule, returns a list of paths to matching files and
directories.

=cut

sub in {
    my $self = _force_object shift;

    my @found;
    my $fragment = $self->_compile;
    my %subs = %{ $self->{subs} };

    warn "relative mode handed multiple paths - that's a bit silly\n"
      if $self->{relative} && @_ > 1;

    my $topdir;
    my $code = 'sub {
        (my $path = $File::Find::name)  =~ s#^(?:\./+)+##;
        my @args = ($_, $File::Find::dir, $path);
        my $maxdepth = $self->{maxdepth};
        my $mindepth = $self->{mindepth};
        my $relative = $self->{relative};

        # figure out the relative path and depth
        my $relpath = $File::Find::name;
        $relpath =~ s{^\Q$topdir\E/?}{};
        my $depth = scalar File::Spec->splitdir($relpath);
        #print "name: \'$File::Find::name\' ";
        #print "relpath: \'$relpath\' depth: $depth relative: $relative\n";

        defined $maxdepth && $depth >= $maxdepth
           and $File::Find::prune = 1;

        defined $mindepth && $depth < $mindepth
           and return;

        #print "Testing \'$_\'\n";

        my $discarded;
        return unless ' . $fragment . ';
        return if $discarded;
        if ($relative) {
            push @found, $relpath if $relpath ne "";
        }
        else {
            push @found, $path;
        }
    }';

    #use Data::Dumper;
    #print Dumper \%subs;
    #warn "Compiled sub: '$code'\n";

    my $sub = eval "$code" or die "compile error '$code' $@";
    for my $path (@_) {
        # $topdir is used for relative and maxdepth
        $topdir = $path;
        # slice off the trailing slash if there is one (the
        # maxdepth/mindepth code is fussy)
        $topdir =~ s{/?$}{}
          unless $topdir eq '/';
        $self->_call_find( { %{ $self->{extras} }, wanted => $sub }, $path );
    }

    return @found;
}

sub _call_find {
    my $self = shift;
    File::Find::find( @_ );
}

sub _compile {
    my $self = shift;

    return '1' unless @{ $self->{rules} };
    my $code = join " && ", map {
        if (ref $_->{code}) {
            my $key = "$_->{code}";
            $self->{subs}{$key} = $_->{code};
            "\$subs{'$key'}->(\@args) # $_->{rule}\n";
        }
        else {
            "( $_->{code} ) # $_->{rule}\n";
        }
    } @{ $self->{rules} };

    #warn $code;
    return $code;
}

=item C<start( @directories )>

Starts a find across the specified directories.  Matching items may
then be queried using L</match>.  This allows you to use a rule as an
iterator.

 my $rule = File::Find::Rule->file->name("*.jpeg")->start( "/web" );
 while ( defined ( my $image = $rule->match ) ) {
     ...
 }

=cut

sub start {
    my $self = _force_object shift;

    $self->{iterator} = [ $self->in( @_ ) ];
    $self;
}

=item C<match>

Returns the next file which matches, false if there are no more.

=cut

sub match {
    my $self = _force_object shift;

    return shift @{ $self->{iterator} };
}

1;

__END__

=back

=head2 Extensions

Extension modules are available from CPAN in the File::Find::Rule
namespace.  In order to use these extensions either use them directly:

 use File::Find::Rule::ImageSize;
 use File::Find::Rule::MMagic;

 # now your rules can use the clauses supplied by the ImageSize and
 # MMagic extension

or, specify that File::Find::Rule should load them for you:

 use File::Find::Rule qw( :ImageSize :MMagic );

For notes on implementing your own extensions, consult
L<File::Find::Rule::Extending>

=head2 Further examples

=over

=item Finding perl scripts

 my $finder = File::Find::Rule->or
  (
   File::Find::Rule->name( '*.pl' ),
   File::Find::Rule->exec(
                          sub {
                              if (open my $fh, $_) {
                                  my $shebang = <$fh>;
                                  close $fh;
                                  return $shebang =~ /^#!.*\bperl/;
                              }
                              return 0;
                          } ),
  );

Based upon this message http://use.perl.org/comments.pl?sid=7052&cid=10842

=item ignore CVS directories

 my $rule = File::Find::Rule->new;
 $rule->or($rule->new
                ->directory
                ->name('CVS')
                ->prune
                ->discard,
           $rule->new);

Note here the use of a null rule.  Null rules match anything they see,
so the effect is to match (and discard) directories called 'CVS' or to
match anything.

=back

=head1 TWO FOR THE PRICE OF ONE

File::Find::Rule also gives you a procedural interface.  This is
documented in L<File::Find::Rule::Procedural>

=head1 EXPORTS

L</find>, L</rule>

=head1 TAINT MODE INTERACTION

As of 0.32 File::Find::Rule doesn't capture the current working directory in
a taint-unsafe manner.  File::Find itself still does operations that the taint
system will flag as insecure but you can use the L</extras> feature to ask
L<File::Find> to internally C<untaint> file paths with a regex like so:

    my $rule = File::Find::Rule->extras({ untaint => 1 });
    
Please consult L<File::Find>'s documentation for C<untaint>,
C<untaint_pattern>, and C<untaint_skip> for more information.

=head1 BUGS

The code makes use of the C<our> keyword and as such requires perl version
5.6.0 or newer.

Currently it isn't possible to remove a clause from a rule object.  If
this becomes a significant issue it will be addressed.

=head1 AUTHOR

Richard Clamp <richardc@unixbeard.net> with input gained from this
use.perl discussion: http://use.perl.org/~richardc/journal/6467

Additional proofreading and input provided by Kake, Greg McCarroll,
and Andy Lester andy@petdance.com.

=head1 COPYRIGHT

Copyright (C) 2002, 2003, 2004, 2006, 2009, 2011 Richard Clamp.  All Rights Reserved.

This module is free software; you can redistribute it and/or modify it
under the same terms as Perl itself.

=head1 SEE ALSO

L<File::Find>, L<Text::Glob>, L<Number::Compare>, find(1)

If you want to know about the procedural interface, see
L<File::Find::Rule::Procedural>, and if you have an idea for a neat
extension L<File::Find::Rule::Extending>

=cut

Implementation notes:

$self->rules is an array of hashrefs.  it may be a code fragment or a call
to a subroutine.

Anonymous subroutines are stored in the $self->subs hashref keyed on the
stringfied version of the coderef.

When one File::Find::Rule object is combined with another, such as in the any
and not operations, this entire hash is merged.

The _compile method walks the rules element and simply glues the code
fragments together so they can be compiled into an anyonymous File::Find
match sub for speed


[*] There's probably a win to be made with the current model in making
stat calls use C<_>.  For

  find( file => size => "> 20M" => size => "< 400M" );

up to 3 stats will happen for each candidate.  Adding a priming _
would be a bit blind if the first operation was C< name => 'foo' >,
since that can be tested by a single regex.  Simply checking what the
next type of operation doesn't work since any arbritary exec sub may
or may not stat.  Potentially worse, they could stat something else
like so:

  # extract from the worlds stupidest make(1)
  find( exec => sub { my $f = $_; $f =~ s/\.c$/.o/ && !-e $f } );

Maybe the best way is to treat C<_> as invalid after calling an exec,
and doc that C<_> will only be meaningful after stat and -X tests if
they're wanted in exec blocks.

package Apbuild::Utils;

use strict;
use warnings;
use Exporter;
use base qw(Exporter);
use IO::Handle;
use IPC::Open2;
use POSIX;
use Cwd qw(abs_path);


our @EXPORT = qw(debug error checkCommand empty homeDir searchLib searchStaticLib soname run parseDataFile writeDataFile);
our $debugOpened = 0;


##
# debug(message)
#
# If the environment variable $APBUILD_DEBUG is set to 1,
# then print a debugging message to /dev/tty (not stdout or stderr).
sub debug {
	return if (empty($ENV{APBUILD_DEBUG}) || !$ENV{APBUILD_DEBUG});

	if (!$debugOpened) {
		if (open DEBUG, '>/dev/tty') {
			$debugOpened = 1;
		} else {
			return;
		}
	}

	my @args = split /\n/, "@_";
	foreach (@args) {
		$_ = '# ' . $_;
		$_ .= "\n";
	}

	print DEBUG "\033[1;33m";
	print DEBUG join '', @args;
	print DEBUG "\033[0m";
	DEBUG->flush;
}


##
# error(message)
#
# Print an error message to stderr. It will be displayed in red.
sub error {
	print STDERR "\033[1;31m";
	print STDERR $_[0];
	print STDERR "\033[0m";
	STDERR->flush;
}


##
# checkCommand(file)
# file: an command's filename.
# Returns: the full path to $file, or undef if $file is not a valid command.
#
# Checks whether $file is an executable which is in $PATH or the working directory.
#
# Example:
# checkCommand('gcc');  # Returns "/usr/bin/gcc"
sub checkCommand {
	my ($file, $file2) = split / /, $_[0];
	$file = $file2 if ($file =~ /ccache/);

	return abs_path($file) if (-x $file);
	foreach my $dir (split /:+/, $ENV{PATH}) {
		if (-x "$dir/$file") {
			return "$dir/$file";
		}
	}
	return undef;
}

##
# empty(str)
#
# Checks whether $str is undefined or empty.
sub empty {
	return !defined($_[0]) || $_[0] eq '';
}

##
# homeDir()
#
# Returns the user's home folder.
sub homeDir {
	if (!$ENV{HOME}) {
		my $user = getpwuid(POSIX::getuid());
		$ENV{HOME} = (getpwnam($user))[7];
	}
	return $ENV{HOME};
}

##
# searchLib(basename, [extra_paths])
# basename: the base name of the library.
# extra_paths: a reference to an array, which contains extra folders in which to look for the library.
# Returns: the absolute path to the library, or undef if not found.
#
# Get the absolute path of a (static or shared) library.
#
# Example:
# searchLib("libfoo.so.1"); # Returns "/usr/lib/libfoo.so.1"
sub searchLib {
	my ($basename, $extra_paths) = @_;

	if ($extra_paths) {
		foreach my $path (reverse(@{$extra_paths})) {
			return "$path/$basename" if (-f "$path/$basename");
		}
	}
	foreach my $path ('/usr/local/lib', '/lib', '/usr/lib') {
		return "$path/$basename" if (-f "$path/$basename");
	}
	return undef;
}

##
# soname(lib)
# lib: a filename to a shared library.
# Returns: the soname.
#
# Get the soname of the specified shared library by reading
# the SONAME section of the shared library file.
sub soname {
	my ($lib) = @_;
	my ($r, $w);

	if (open2($r, $w, 'objdump', '-p', $lib)) {
		close $w;
		my @lines = <$r>;
		close $r;

		my ($soname) = grep {/SONAME/} @lines;
		$soname =~ s/.*?SONAME[ \t]+//;
		$soname =~ s/\n//gs;
		return $soname;

	} else {
		my ($soname) = $lib =~ /.*\/lib(.+)\.so/;
		return $soname;
	}
}

##
# run(args...)
# Returns: the command's exit code.
#
# Run a command with system().
sub run {
	# split the first item in @_ into "words".  The `printf ...`
	# takes care of respecting ' and " quotes so we don't split a
	# quoted string that contains whitespace.  If $cmd itself
	# contains \n, this will still go wrong.
	my $cmd    = shift @_;
	my @words  = `printf '%s\n' $cmd`;
	chomp @words;
	my $status = system(@words, @_);
	return 127 if ($status == -1);
	return $status / 256 if ($status != 0);
	return 0;
}

sub parseDataFile {
	my ($file, $r_hash) = @_;

	%{$r_hash} = ();
	return if (!open FILE, "< $file");
	foreach (<FILE>) {
		next if (/^#/);
		s/[\r\n]//g;
		next if (length($_) == 0);

		my ($key, $value) = split /	/, $_, 2;
		$r_hash->{$key} = $value;
	}
	close FILE;
}

sub writeDataFile {
	my ($file, $r_hash) = @_;
	return if (!open FILE, "> $file");
	foreach my $key (sort(keys %{$r_hash})) {
		print FILE "$key	$r_hash->{$key}\n";
	}
	close FILE;
}

1;

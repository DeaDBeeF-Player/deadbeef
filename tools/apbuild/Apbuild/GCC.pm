# Various gcc-related functions
package Apbuild::GCC;

use strict;
use Apbuild::Utils;
use IPC::Open3;


######## Special constants; used for parsing GCC parameters ########

# Parameters that require an extra parameter
our $extraTypes = 'o|u|Xlinker|b|V|MF|MT|MQ|I|L|R|Wl,-rpath|isystem|D|x';
# Linker parameters
our $linkerTypes = 'L|Wl|o$|l|s|n|R';
# Files with these extensions are objects
our $staticLibTypes = 'a|al';
our $objectTypes = "o|os|so(\\.\\d+)*|la|lo|$staticLibTypes";
our $headerTypes = 'h|hpp';
# Source file types
our $cTypes = 'c|C';
our $cxxTypes = 'cpp|cxx|cc|c\+\+';
our $srcTypes = "$cTypes|$cxxTypes";


######## Methods ########

##
# Apbuild::GCC->new(gcc_command)
# gcc_command: The command for invoking GCC.
#
# Create a new Apbuild:GCC object.
sub new {
	my ($class, $gcc) = @_;
	my %self;

	$self{gcc} = $gcc;
	$self{gcc_file} = checkCommand($gcc);
	if (!defined $self{gcc_file}) {
		error "$gcc: command not found\n";
		exit 127;
	}

	# FIXME: should use gcc -print-search-paths.
	$self{searchPaths} = ["/usr/lib", "/usr/local/lib"];
	foreach (reverse(split(/:/, $ENV{LIBRARY_PATH}))) {
		push @{$self{searchPaths}}, $_;
	}

	bless \%self, $class;
	return \%self;
}

##
# $gcc->capabilities()
# Returns: a hash with capabilities for this compiler.
#
# Check GCC's capabilities. This function will return
# a hash with the following keys:
# libgcc   : Is 1 if gcc supports the parameter -{shared,static}-libgcc.
# as_needed: Is 1 if gcc's linker supports --as-needed.
# abi2     : Is 1 if gcc supports -fabi-version=2.
#
# This function takes care of caching results.
sub capabilities {
	my ($self) = @_;
	my (%capabilities, %cache);

	my $gcc = $self->{gcc};
	my $gcc_file = $self->{gcc_file};

	# First, check the cache
	my @stat = stat $gcc_file;
	my $gcc_mtime = $stat[9];
	my $home = homeDir();
	if (-f "$home/.apbuild") {
		parseDataFile("$home/.apbuild", \%cache);
		if ($cache{version} != 2) {
			# Cache file version incompatible; delete cache
			%cache = ();

		} else {
			if ($cache{"mtime_$gcc_file"} != $gcc_mtime) {
				# Cache out of date for this compiler; update cache
				delete $cache{"libgcc_$gcc_file"};
				delete $cache{"abi2_$gcc_file"};
				delete $cache{"as_needed_$gcc_file"};
				delete $cache{"hash_style_$gcc_file"};
				delete $cache{"stack_protector_$gcc_file"};
				delete $cache{"fortify_source_$gcc_file"};
			}
		}
	}

	my $lc_all = $ENV{LC_ALL};
	$ENV{LC_ALL} = 'C';

	if (exists $cache{"libgcc_$gcc_file"} && exists $cache{"abi2_$gcc_file"}) {
		$capabilities{libgcc} = $cache{"libgcc_$gcc_file"};
		$capabilities{abi2} = $cache{"abi2_$gcc_file"};

	} else {
		# Get output from 'gcc -v'
		my ($r, $w, $e);

		my $pid = open3($w, $r, $e, $gcc_file, '-v');
		close $w if ($w);
		close $e if ($e);
		my @output = <$r>;
		waitpid $pid, 0;

		# Check whether gcc >= 3.0
		my ($major, $minor) = $output[@output - 1] =~ /version ([0-9]+)\.([0-9]+)/;
		if ($major >= 3) {
			$capabilities{libgcc} = 1;
			$cache{"libgcc_$gcc_file"} = 1;
		} else {
			$capabilities{libgcc} = 0;
			$cache{"libgcc_$gcc_file"} = 0;
		}

		if ($major > 3 || ($major >= 3 && $minor >= 4)) {
			$capabilities{abi2} = 1;
			$cache{"abi2_$gcc_file"} = 1;
		} else {
			$capabilities{abi2} = 0;
			$cache{"abi2_$gcc_file"} = 0;
		}

		$cache{"mtime_$gcc_file"} = $gcc_mtime;
	}

	if (exists $cache{"as_needed_$gcc_file"}) {
		$capabilities{as_needed} = $cache{"as_needed_$gcc_file"};

	} else {
		my ($r, $w, $e);

		my $pid = open3($w, $r, $e, $gcc_file, '-Wl,--help');
		close $w if ($w);
		close $e if ($e);
		local($/);
		my $output = <$r>;
		waitpid $pid, 0;

		if ($output =~ /--as-needed/) {
			$capabilities{as_needed} = 1;
			$cache{"as_needed_$gcc_file"} = 1;
		} else {
			$capabilities{as_needed} = 0;
			$cache{"as_needed_$gcc_file"} = 0;
		}
	}

	if (exists $cache{"hash_style_$gcc_file"}) {
		$capabilities{hash_style} = $cache{"hash_style_$gcc_file"};
	} else {
		my ($r, $w, $e);

		my $pid = open3($w, $r, $e, $gcc_file, '-Wl,--help');
		close $w if ($w);
		close $e if ($e);
		local($/);
		my $output = <$r>;
		waitpid $pid, 0;

		if ($output =~ /--hash-style/) {
			$capabilities{hash_style} = 1;
			$cache{"hash_style_$gcc_file"} = 1;
		} else {
			$capabilities{hash_style} = 0;
			$cache{"hash_style_$gcc_file"} = 0;
		}
	}
	
	if (exists $cache{"stack_protector_$gcc_file"}) {
		$capabilities{stack_protector} = $cache{"stack_protector_$gcc_file"};
	} else {
		# Get output from 'gcc -v'
		my ($r, $w, $e);

		my $pid = open3($w, $r, $e, $gcc_file, '-v');
		close $w if ($w);
		close $e if ($e);
		my @output = <$r>;
		waitpid $pid, 0;

		# Check whether gcc >= 3.0
		my ($major, $minor) = $output[@output - 1] =~ /version ([0-9]+)\.([0-9]+)/;
		if ($major >= 4 && $minor >= 1) {
			$capabilities{stack_protector} = 1;
			$cache{"stack_protector_$gcc_file"} = 1;
		} else {
			$capabilities{stack_protector} = 0;
			$cache{"stack_protector_$gcc_file"} = 0;
		}
	}

	if (exists $cache{"fortify_source_$gcc_file"}) {
		$capabilities{fortify_source} = $cache{"fortify_source_$gcc_file"};
	} else {
		# Get output from 'gcc -v'
		my ($r, $w, $e);

		my $pid = open3($w, $r, $e, $gcc_file, '-v');
		close $w if ($w);
		close $e if ($e);
		my @output = <$r>;
		waitpid $pid, 0;

		# Check whether gcc >= 4.1
		my ($major, $minor) = $output[@output - 1] =~ /version ([0-9]+)\.([0-9]+)/;
		if ($major >= 4 && $minor >= 1) {
			$capabilities{fortify_source} = 1;
			$cache{"fortify_source_$gcc_file"} = 1;
		} else {
			$capabilities{fortify_source} = 0;
			$cache{"fortify_source_$gcc_file"} = 0;
		}
	}

	if (defined $lc_all) {
		$ENV{LC_ALL} = $lc_all;
	} else {
		delete $ENV{LC_ALL};
	}

	$cache{version} = 2;
	writeDataFile("$home/.apbuild", \%cache);
	return %capabilities;
}


##
# $gcc->command()
#
# Returns the GCC command associated with this Apbuild::GCC object.
sub command {
	my ($self) = @_;
	return $self->{gcc};
}


##
# $gcc->foreach(args, callback)
# args: a reference to an array which contains GCC parameters.
# callback: the callback function which will be called in every iteration of the loop.
#
# Iterate through each GCC parameter. $callback will be called as follows:
#     $callback->(type, args...);
# $type is the current parameter's type, either "param" or "file" (source file).
# args is an array of parameters. If this parameter is a parameter which excepts
# another parameter (such as -o, it expects a filename), then the expected
# parameter will also be passed to the callback function.
sub foreach {
	my ($self, $args, $callback) = @_;
	for (my $i = 0; $i < @{$args}; $i++) {
		$_ = $args->[$i];
		if (/^-/ || /\.($objectTypes|$headerTypes)$/ || /.*\.so\.?$/) {
			# Parameter
			if (/^-($extraTypes)$/) {
				# This parameter expects another parameter
				$callback->("param", $_, $args->[$i + 1]);
				$i++;
			} else {
				$callback->("param", $_);
			}

		} else {
			# File
			$callback->("file", $_);
		}
	}
}

##
# $gcc->splitParams(args, files, params)
# args: a reference to an array which contains GCC parameters.
# files: a reference to an array, or undef.
# params: a reference to an array, or undef.
#
# Parse GCC's parameters. Seperate files and arguments.
# A list of files will be stored in the array referenced to by $file,
# a list of non-file parameters will be stored in the array referenced to by $params.
sub splitParams {
	my ($self, $args, $files, $params) = @_;

	my $callback = sub {
		my $type = shift;
		if ($type eq "param") {
			push @{$params}, @_ if ($params);
		} elsif ($type eq "file") {
			push @{$files}, $_[0] if ($files);
		}
	};
	$self->foreach($args, $callback);
}


sub stripLinkerParams {
	my ($self, $r_params, $linking) = @_;
	my @params;
	my $i = 0;

	my $callback = sub {
		my $type = shift;
		if ($type eq 'param' && $_[0] =~ /^-($linkerTypes)/) {
			push @{$linking}, @_;
		} else {
			push @params, @_;
		}
	};
	$self->foreach($r_params, $callback);
	@{$r_params} = @params;
}


sub addSearchPaths {
	my ($self, $dir) = @_;
	push @{$self->{searchPaths}}, $dir;
}

# Parse $args and extract library search paths from it.
# Those paths, along with paths added by addSearchPaths(),
# will be appended to $paths.
sub getSearchPaths {
	my ($self, $args, $paths) = @_;

	my $callback = sub {
		my $type = shift;
		return if ($type ne "param");

		if ($_[0] eq "-L") {
			push @{$paths}, $_[1];

		} elsif ($_[0] =~ /^-L(.+)/ || $_[0] =~ /^--library-path=(.+)/) {
			push @{$paths}, $1;
		}
	};
	$self->foreach($args, $callback);

	push @{$paths}, @{$self->{searchPaths}};
}


##
# $gcc->isLibrary(arg, libname)
#
# Check whether $arg is a (dynamic) library argument.
# The base library name will be stored in $$libname.
sub isLibrary {
	my ($self, $arg, $libname) = @_;
	if ($arg =~ /^-l(.+)/ || $arg =~ /^--library=(.+)/ || $arg =~ /^(?:.*\/)?lib([^\/]+)\.so/) {
		$$libname = $1 if ($libname);
		return 1;
	} else {
		return 0;
	}
}

##
# $gcc->isObject(arg)
#
# Check whether $arg is an object. This includes static libraries.
sub isObject {
	my ($self, $arg) = @_;
	return $arg !~ /^-/ && $arg =~ /\.($objectTypes)$/;
}

sub isStaticLib {
	my ($self, $arg) = @_;
	return $arg !~ /^-/ && $arg =~ /\.($staticLibTypes)$/;
}

##
# $gcc->isSource(file)
#
# Checks whether $file is a source file.
sub isSource {
	my ($self, $file) = @_;
	return $file =~ /\.($srcTypes)$/;
}

##
# $gcc->isCxxSource(file)
#
# Checks whether $file is a C++ source file.
sub isCxxSource {
	my ($self, $file) = @_;
	return $file =~ /\.($cxxTypes)$/;
}


# I wish I know how to explain what these functions do, but I can't. :(
# If you know a better way to name these, please do tell me.
sub sourceOrderIsImportant {
	my ($self, $arg) = @_;
	return $arg =~ /^-(x)$/;
}

sub linkOrderIsImportant {
	my ($self, $arg) = @_;
	return $arg =~ /^-(Wl,--whole-archive|Wl,--no-whole-archive)$/;
}

##
# $gcc->getOutputFile(args)
# args: a reference to an array which contains GCC parameters.
# Returns: a filename.
#
# Parse the GCC arguments and detect the output filename.
sub getOutputFile {
	my ($gcc, $args) = @_;
	my ($output, $compilingToObject, $source);

	my $callback = sub {
		my $type = shift;
		if ($type eq 'param' && $_[0] eq '-o') {
			$output = $_[1];

		} elsif ($type eq 'param' && $_[0] eq '-c') {
			$compilingToObject = 1;

		} elsif ($type eq 'file' && $gcc->isSource($_[0])) {
			$source = $_[0];
		}
	};
	$gcc->foreach($args, $callback);

	if (defined $output) {
		return $output;
	} elsif ($compilingToObject) {
		$source =~ s/\.[^.]*?$/.o/;
		return $source;
	} else {
		return "a.out";
	}
}

##
# $gcc->setOutputFile(args, new_output_file)
# args: a reference to an array which contains GCC parameters.
# new_output_file: the new output filename.
#
# Parse the GCC arguments and change the output filename.
# The array referenced by $args will be modified.
sub setOutputFile {
	my ($gcc, $args, $new_output_file) = @_;
	my (@newArgs, $changed);

	my $callback = sub {
		my $type = shift;

		if ($type eq 'param' && $_[0] eq '-o') {
			push @newArgs, "-o", $new_output_file;
			$changed = 1;

		} else {
			push @newArgs, @_;
		}
	};
	$gcc->foreach($args, $callback);

	if (!$changed) {
		push @newArgs, "-o", $new_output_file if (!$changed);
	}
	@{$args} = @newArgs;
}


##
# $gcc->situation(args)
# args: a reference to an array which contains GCC arguments.
# Returns: 'compile', 'depcheck', 'compile and link', 'linking' or 'other'.
#
# Detect the situation in which the compiler is used.
# Basically, there are 5 situations in which the compiler is used:
# 1) Compilation (to an object file).
# 2) Linking.
# 3) Compilation and linking.
# 4) Dependancy checking with -M* or -E.
# 5) None of the above. Compiler is invoked with --help or something.
# Note that source files may also contain non-C/C++ files.
sub situation {
	my ($self, $args) = @_;

        my $files = 0;
        for (@{$args}) {
                $files++ if (!(/^-/));
        }

	for (@{$args}) {
		if (/^-c$/) {
			# Situation 1
			return 'compile';
		} elsif (/^-M(|M|G)$/ || /^-E$/) {
			# Situation 4
			return 'depcheck';
		}
	}

	if ($files == 1)
	{
		my $i = 0;
		for (@{$args})
		{
			if (!(/^-/) && (/\.($headerTypes)$/))
			{
				print($args->[$i], "\n");
				return 'precompiled header';
			}
			$i++;
		}
	}

	my $i = 0;
	for (@{$args}) {
		if (!(/^-/) && !(/\.($objectTypes)$/)) {
			if ($i > 0 && $args->[$i - 1] =~ /^-($extraTypes)$/) {
				$i++;
				next;
			} else {
				# Situation 3
				return 'compile and link';
			}
		}
		$i++;
	}

	if ($files == 0) {
		# Situation 5
		return 'other';
	} else {
		# Situation 2
		return 'linking';
	}
}


1;

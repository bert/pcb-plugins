#!/usr/bin/perl

use strict;

sub usage() {
	print STDERR "usage: getFileNames.pl path dstExtension srcExtension0 srcExtension1 ...\n";
	exit 1;
}

sub getNames($$$) {
	my ($path, $dstExt, $srcExt) = @_;
	my @find = `find $path -name '*.$srcExt'`;
	foreach my $srcPath (@find) {
			if ($srcPath =~ s/$srcExt$/$dstExt/) {
				print "$srcPath";
			}
	}
}

my $path = shift @ARGV or usage();
my $dstExt = shift @ARGV or usage();
my @srcExtList = @ARGV;

foreach my $srcExt (@srcExtList) {
	getNames($path, $dstExt, $srcExt);
}

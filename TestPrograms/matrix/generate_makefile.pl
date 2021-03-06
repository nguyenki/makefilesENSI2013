#!/usr/bin/env perl
	
use strict;
use warnings;

my $size = $ARGV[0];
$size = 10 unless defined $size;

my @a;
my @b;
my @c;
my @p;

print "all: c\n";
#print "all: c check\n\n";
print "\tcat c > all\n";
print "check:\ta b\n";
print "\t multiply check a b\n";

my ($i, $j, $k);
foreach $i (1..$size) {
	foreach $j (1..$size) {
		push @a, "a-$i-$j";
		push @b, "b-$i-$j";
		push @c, "c-$i-$j";
		foreach $k (1..$size) {
			push @p, "p-$i-$k-$k-$j";
		}
	}
}

print "c:\t@c\n";
print "\tfuse c $size $size @c\n";

print "\n#\n";

my $c;
foreach $c (@c) {
	my @chars = split(/-/, $c);
	my $i = $chars[1];
	my $j = $chars[2];
	my @deps;
	push @deps, "p-$i-$_-$_-$j" foreach (1..$size);
	print "$c:\t@deps\n";
	print "\tsum $c @deps\n";
}

print "\n#\n";

my $p;
foreach $p (@p) {
	my @chars = split(/-/, $p);
	my $i = $chars[1];
	my $k = $chars[2];
	my $j = $chars[4];
	print "$p:\ta-$i-$k b-$k-$j\n";
	print "\tmultiply $p a-$i-$k b-$k-$j\n"
}

print "\n#\n";

foreach $a (@a) {
	my @chars = split(/-/, $a);
	my $i = $chars[1];
	my $j = $chars[2];
	print "$a:\ta\n";
	print "\tsplit $a a $size $size $i $j\n";
}

print "\n#\n";

foreach $b (@b) {
	my @chars = split(/-/, $b);
	my $i = $chars[1];
	my $j = $chars[2];
	print "$b:\tb\n";
	print "\tsplit $b b $size $size $i $j\n";
}

print "clean:\n";
print "\trm -f @a @b @p @c c check\n";

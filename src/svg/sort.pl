#!/usr/bin/perl

if($#ARGV != 0) {
    print "Usage: sort.pl filename\n";
    exit;
}

my $filename = $ARGV[0];
open(TXTFILE, '<', $filename)
or die "Could not open file '$filename' $!";

my $line;
my @s1;
my @s2;

while (<TXTFILE>) {
	if(m/([a-zA-Z]+-?[a-zA-Z]*[0-9]*)/) {
    	# print "--  $1\n";
	  	$line = lc $1;		
		push @s1, $line;
	}
}
@s2 = sort @s1;

foreach (@s2) {
	print "PG_KEYWORD(\"$_\", ABORT_P, UNRESERVED_KEYWORD, BARE_LABEL)\n";
}

close(TXTFILE);

exit;

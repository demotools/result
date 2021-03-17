#!/usr/bin/perl

# Copyright 2009, Princeton University
# All rights reserved.
#
# A simple script to generate inputs for the canneal workload of the PARSEC
# Benchmark Suite.

use strict;
my @names;

my $x = 5;
my $y = 5;
my $num_elements = 10;
($x > 1) or die "x is invalid: $y";
($y >1) or die "y is invalid: $y";
($num_elements < ($x * $y) )or die;
my $num_connections = 5;

print "$num_elements	$x	$y\n";

my $openFlag;
my @lines;
$openFlag = open(FDW, ">>native.nets");  

printf(FDW "$num_elements	$x	$y\n");

if($openFlag){  

	my $name = "a";
foreach my $i (0..$num_elements-1){
	$names[$i] = $name;
	$name++;
}

foreach my $i (0..$num_elements-1){
	# print "$names[$i]\t";
	printf(FDW "$names[$i]\t");
	#type is either reg or comb  For now my program makes no distinction
	my $type = 1+ int(rand(2));
	# print "$type\t";
	printf(FDW "$type\t");
	foreach my $j (0..$num_connections-1){
		#get a random element
		my $random_connection = int(rand($num_elements));
		printf(FDW "$names[$random_connection]");
		printf(FDW "\t");
		# print $names[$random_connection];
		# print "\t";
	}
	printf(FDW "END\n");
	# print "END\n";
}

    print "打开文件成功\n";      

    # @lines =<FD1>;  

    # # print "打开文件内容:\n";  

    # # print "@lines\n";  

    # open(FDW, ">>native.nets");  

    # printf(FDW " @lines");  

    close FDW;  
    close FD1;  
}else{  
    die "打开文件失败: $!\n";  
} 

#create a set of names.  Use the ++ operator gives meaningless names, but thats 
#all I really need
# my $name = "a";
# foreach my $i (0..$num_elements-1){
# 	$names[$i] = $name;
# 	$name++;
# }

# foreach my $i (0..$num_elements-1){
# 	print "$names[$i]\t";
# 	#type is either reg or comb  For now my program makes no distinction
# 	my $type = 1+ int(rand(2));
# 	print "$type\t";
# 	foreach my $j (0..$num_connections-1){
# 		#get a random element
# 		my $random_connection = int(rand($num_elements));
# 		print $names[$random_connection];
# 		print "\t";
# 	}
# 	print "END\n";
# }

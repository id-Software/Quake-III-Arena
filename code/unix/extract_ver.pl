#!/usr/bin/env perl
# extracting version string from game/q_shared.h
# hacked from Wolf build process

# extract the wolf version from q_shared.h
$line = `cat ../game/q_shared.h | grep Q3_VERSION`;
chomp $line;
$line =~ s/.*Q3\ (.*)\"/$1/;
print "$line\n";

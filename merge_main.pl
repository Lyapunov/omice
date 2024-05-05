#!/usr/bin/perl -w

use strict;

my ( $mainFile, $resultFile ) = @ARGV;
die "USAGE: $0 <main.cpp>\n" if !$mainFile;

my %imap = %{ getIncludeMap( $mainFile ) };
print "Dependencies:\n";
for my $key ( keys %imap ) {
   print "$key => ".join(",", keys %{$imap{$key}})."\n";
}
my @topOrd = topologicalOrder(\%imap);
print"\n";
print+"Order to merge:\n";
print+join(",", @topOrd)."\n";

my %prags;
for my $ifile ( @topOrd ) {
   %prags = (%prags, map { $_ => 1 } `grep "#pragma" $ifile`);
}
my %includes;
for my $ifile ( @topOrd ) {
   %includes = (%includes, map { $_ => 1 } `grep "#include <" $ifile`);
}
open(OFILE,">$resultFile");
for my $prag ( sort keys %prags ) {
   print OFILE $prag;
}
for my $inc ( sort keys %includes ) {
   print OFILE $inc;
}
my $emptyLine = 0;
# merge includes first
for my $ifile ( @topOrd ) {
   open(IFILE, "<$ifile");
   while (<IFILE>) {
      next if m/#include/;
      next if m/#pragma/;
      next if m/#ifndef/;
      next if m/#endif/;
      next if m/#define/;
      next if m/std::cerr.*std::endl/;
      next if m!^\s*//!;
      print OFILE $_ unless (m/^$/ && $emptyLine);
      $emptyLine = m/^$/;
   }
   close(IFILE);
}

close(OFILE);

print "\nBaking $resultFile is complete.\n";

sub getIncludes {
   my ($file) = @_;
   my %res = ();
   my $cppFile = $file;
   $cppFile =~ s!\.hpp!.cpp!g;
   for my $line ( `grep '^#include "' $file`, `grep '^#include "' $cppFile`  ) {
      if ( $line =~ m/"([^"]*)"/ ) {
         if ( $1 ne $cppFile && $1 ne $file ) {
            $res{$1} = 1;
         }
      }
   }
   return \%res;
}

sub getIncludeMap {
   my ($file) = @_;
   my %imap;
   my %sset = %{getIncludes($file)};
   $imap{$file} = \%sset;
   my %currSet = %sset;
   my %nextSet = ();
   while ( keys %currSet ) {
      %nextSet = ();
      for my $curr ( sort keys %currSet ) {
         if ( !$imap{$curr} ) {
            my %set = %{ getIncludes( $curr ) };
            $imap{$curr} = \%set;
            %nextSet = (%nextSet, %set);
         }
      }
      %currSet = %nextSet;
   }
   return \%imap;
}

sub topologicalOrder {
   my ($refImap) = @_;
   my %imap = %{ $refImap };
   my @retval;
   my $hist = 0;
   while ( %imap ) {
      my ( $candid ) = grep { !%{ $imap{$_} } } keys %imap;
      if ( !$candid ) {
         return undef;
      }
      push @retval, $candid;
      if ( $candid =~ m/\.hpp/ ) {
         my $cppFile = $candid;
         $cppFile =~ s/\.hpp/.cpp/g;
         push @retval, $cppFile;
      }
      delete $imap{$candid};
      for my $key ( keys %imap ) {
         delete ${ $imap{$key} }{ $candid };
      }
   }
   return @retval;
}

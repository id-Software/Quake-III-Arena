#
# Some utilities to handle gcc compiler setup
#

package Cons_gcc;

# pass the compiler name
# returns an array, first element is 2 for 2.x 3 for 3.x, then full version, then machine info
sub get_gcc_version
{
  my @ret;
  my ($CC) = @_;
  my $version=`$CC --version | head -1`;
  chop($version);
  my $machine=`$CC -dumpmachine`;
  chop($machine);
  if($version =~ '2\.[0-9]*\.[0-9]*')
  {
    push @ret, '2';
  } else {
    push @ret, '3';
  }
  push @ret, $version;
  push @ret, $machine;
  return @ret;
}

# http://ccache.samba.org/
# check ccache existence and path
# returns an array, first element 0 / 1, then path
sub get_ccache
{
  my @ret;  
  $ccache_path=`which ccache`;
  chop($ccache_path);
  if(-x $ccache_path)
  {
    push @ret, '1';
    push @ret, $ccache_path;
    return @ret;
  }
  push @ret, '0';
  return @ret;
}

# close package
1;

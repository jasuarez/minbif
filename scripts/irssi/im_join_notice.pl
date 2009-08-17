#CHANGELOG:
#
#17-08-2009:
#modified so it works with minbif too
#
#28-11-2004:
#it gives a join message in a query if a user joins &bitlbee and you hve a query open with that person.
#
#/statusbar window add join_notice
#use Data::Dumper;

use strict;
use Irssi::TextUI;
#use Irssi::Themes;

use vars qw($VERSION %IRSSI);

$VERSION = '2.0';
%IRSSI = (
    authors     => 'Tijmen "timing" Ruizendaal, Sebastien Delafond',
    contact     => 'tijmen.ruizendaal@gmail.com, seb@debian.org',
    name        => 'IM_join_notice',
    description => '	1. Adds an item to the status bar wich shows [joined: <nicks>] when someone is joining on IM.
    			2. Shows join messages in the query.',
    license => 'GPLv2',
    url         => 'http://symlink.me/repositories/show/minbif/scripts/irssi',
    changed => '2009-08-17',
);

my %online;
my %tag;
my @control_channels = ();

#my $debug_window = Irssi::window_find_item("(init)");

get_channels();

sub get_channels {
  my @channels = Irssi::channels();
  foreach my $channel (@channels) {
    my $name = $channel->{name};
    my $topic = $channel->{topic};
    my $type = $channel->{server}->{tag};
    if (($name =~ m/^&/) && (($topic eq "Welcome to the control channel. Type \x02help\x02 for help information.") || ($type eq "minbif"))) {
#      $debug_window->print("$name, $type\n");
      push(@control_channels, ($name, $type));
    }
  }
}

sub event_join {
  my ($server, $channel, $nick, $address) = @_;
#  $debug_window->print("$server, $channel, $nick, $address\n");
  if (grep {$_ eq ($channel, $server->{tag})} @control_channels) {
    $online{$nick} = 1;
    Irssi::timeout_remove($tag{$nick});
    $tag{$nick} = Irssi::timeout_add(7000, 'empty', $nick);
    Irssi::statusbar_items_redraw('join_notice');
    my $window = Irssi::window_find_item($nick);
    if($window){
      $window->printformat(MSGLEVEL_JOINS, 'join', $nick, $address, $channel); 
    }
  }
}
sub join_notice {
  my ($item, $get_size_only) = @_; 
  my $line;
  foreach my $key (keys(%online)) {
    $line = $line." ".$key;
  }
  if ($line ne "") {
    $item->default_handler($get_size_only, "{sb joined:$line}", undef, 1);
    $line = "";
  } else {
    $item->default_handler($get_size_only, "", undef, 1);
  } 
}

sub empty{
  my $nick = shift;
  delete($online{$nick});
  Irssi::timeout_remove($tag{$nick});
  Irssi::statusbar_items_redraw('join_notice');
}

Irssi::signal_add_last('channel sync', 'get_channels');
Irssi::signal_add("event join", "event_join");

Irssi::statusbar_item_register('join_notice', undef, 'join_notice');
Irssi::statusbars_recreate_items();

Irssi::theme_register([
  'join', '-->> {channick_hilight $0} {chanhost $1} has joined {channel $2}',
]);

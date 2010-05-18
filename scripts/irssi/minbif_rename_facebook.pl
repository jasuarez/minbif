#!/usr/bin/perl

use strict;

use Irssi;
use Irssi::Irc;

use vars qw( $VERSION %IRSSI );

($VERSION) = '$Revision: 0.04 $' =~ m{ (\d+[.]\d+) };

%IRSSI = (
    name        => 'Minbif Facebook Renamer',
    authors     => 'Justin Wheeler',
    contact     => 'minbif@datademons.com',
    license     => 'GPL',
    description => q{Program that will try and rename people on your Facebook }
                 . q{from jabber's default u<userid> to their real name.}
);

my $minbif_channel  = '&minbif';
my $facebook_server = qr/chat[.]facebook[.]com/i;
my $unnamed_pattern = qr/^[u-]\d+/;

my %minbif_servers;
my %changed_nicks;
my %attempted_to_rename;

sub message_join {
    my ($server, $channel, $nick, $address) = @_;

    return if $channel ne $minbif_channel
           || $address !~ $facebook_server
           || $nick    !~ $unnamed_pattern;

    $minbif_servers{ $server->{ tag } } = 1;
    $attempted_to_rename{ $nick }       = q{};

    $server->redirect_event(
        'whois', 1, ":$nick", -1, undef,
        {
            'event 311' => 'redir whois_data',
            'event 312' => 'redir ignore_it',
            'event 319' => 'redir ignore_it',
            'event 320' => 'redir extended_data',
        },
    );

    $server->send_raw("WHOIS $nick $nick");

    return;
}

sub whois_data {
    my ($server, $data) = @_;

    my ($me, $nick, $user, $host, $star, $real_name) = split /\s+/, $data, 6;

    return if !exists $minbif_servers{ $server->{ tag } }
           || $nick !~ $unnamed_pattern
           || !exists $attempted_to_rename{ $nick };

    if (my ($actual_name) = $real_name =~ m{:(.+)(?=\s+\[)}) {
        if ($actual_name !~ $unnamed_pattern && $actual_name ne q{}) {
           _change_nickname( $server, $nick, $actual_name );
       }
    }

    Irssi::signal_stop();

    return;
}

sub _change_nickname {
    my ($server, $old_nick, $new_nick) = @_;

    $new_nick = _clean_nick( $new_nick );

    foreach my $changed_nick ( keys %changed_nicks ) {
        delete $changed_nicks{ $changed_nick }
            if (time - $changed_nicks{ $changed_nick }) > 10;

        delete $changed_nicks{ $changed_nick }
               if $attempted_to_rename{ $changed_nick } ne $new_nick;
    }

    return if exists $changed_nicks{ $old_nick };

    $changed_nicks{ $old_nick } = time;

    $attempted_to_rename{ $old_nick } = $new_nick;

    Irssi::print("Renaming $old_nick to $new_nick");

    $server->command("quote SVSNICK $old_nick $new_nick");

    return;
}

sub nick_used {
    my ($server, $data) = @_;

    return if !exists $minbif_servers{ $server->{ tag } };

    my ($nick, $new_nick, $used_message) = split /\s+/, $data, 3;

    my ( $old_nick ) = grep { $attempted_to_rename{ $_ } eq $new_nick }
                            keys %attempted_to_rename;

    return if !$old_nick;

    if ($new_nick eq substr( "${old_nick}_Facebook", 0, 29 )) {
        Irssi::print(
            qq{I tried renaming $old_nick to $new_nick with and without a }
          .  q{facebook suffix, but was unsuccessful.  You'll need to rename }
          .  q{this user manually.}
        );

        return;
    }

    Irssi::print("$new_nick appears to be used -- trying ${new_nick}_Facebook");

    _change_nickname( $server, $old_nick, "${new_nick}_Facebook" );

    Irssi::signal_stop();

    return;
}

sub extended_data {
    my ($server, $data) = @_;

    return if !exists $minbif_servers{ $server->{ tag } };

    my ($me, $nick, $rest) = split /\s+/, $data, 3;

    return if $nick !~ $unnamed_pattern;

    if ($rest =~ s/:Full\s+Name:\s+//) {
        return if !$rest;

        _change_nickname( $server, $nick, $rest );
    }

    return;
}

sub _clean_nick {
    my ($name) = @_;

    $name =~ s/[^\w\d_-]+/_/g;
    $name =~ s/_{2,}/_/g;

    # Minbif's nick length limit.
    return substr( $name, 0, 29 );
}

sub nick_change {
    my ($server, $new_nick, $old_nick, $host) = @_;

    return if !exists $minbif_servers{ $server->{ tag } };

    delete $attempted_to_rename{ $old_nick };

    return;
}

sub ignore_it {
    my ($server, $info) = @_;

    return if !exists $minbif_servers{ $server->{ tag } };

    my ($me, $them, $everything_else) = split /\s+/, $info, 3;

    # If we just WHOIS'd them, but haven't tried renaming yet.
    if (exists $attempted_to_rename{ $them }) {
        Irssi::signal_stop();
    }

    return;
}

Irssi::signal_add_first('message join'  => 'message_join' );
Irssi::signal_add_first('message nick'  => 'nick_change'  );
Irssi::signal_add_first('whois_data'    => 'whois_data'   );
Irssi::signal_add_first('extended_data' => 'extended_data');
Irssi::signal_add_first('event 433'     => 'nick_used'    );
Irssi::signal_add_first('event 311'     => 'whois_data'   );
Irssi::signal_add_first('event 312'     => 'ignore_it'    );
Irssi::signal_add_first('event 319'     => 'ignore_it'    );
Irssi::signal_add_first('event 320'     => 'extended_data');

/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "token.hpp"

namespace eosio {

const auto EOS_SYMBOL = eosio::symbol("EOS", 4);
const auto MIC_SYMBOL = eosio::symbol("MIC", 4);

void token::init(){
    require_auth( _self );

    auto t = asset(0, eosio::symbol("MIC", 4));
    auto sym = t.symbol;
    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    const auto& st = *existing;

    statstable.modify( st, _self, [&]( auto& s ) {
       s.max_supply.amount    = s.max_supply.amount + 9000;
    });
}

void token::create( name issuer,
                    asset        maximum_supply )
{
    require_auth( get_self() );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( get_self(), [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer.value;
    });
}


void token::issue( name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto sym_name = sym.code().raw();
    stats statstable( get_self(), sym_name );
    auto existing = statstable.find( sym_name );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( name(st.issuer) );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, get_self(), [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( name(st.issuer), quantity, name(st.issuer) );

    if( to != name(st.issuer) ) {
       auto x = name(st.issuer);
       SEND_INLINE_ACTION( *this, transfer, {x, "active"_n}, {x, to, quantity, memo} );
    }
}

void token::transfer( name from,
                      name to,
                      asset        quantity,
                      string       memo )
{
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.code().raw();
    stats statstable( get_self(), sym );
    const auto& st = statstable.get( sym );

    require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );


    sub_balance( from, quantity );
    add_balance( to, quantity, from );
}

void token::sub_balance( name owner, asset value ) {
   accounts from_acnts( get_self(), owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );


   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
      });
   }
}

void token::add_balance( name owner, asset value, name ram_payer )
{
   accounts to_acnts( get_self(), owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, get_self(), [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void token::onTransfer(name from, name to, asset eos, std::string memo) {        
    
    if (to != _self) {
        return;
    }

    // eosio_assert(from == N(myeosgroupon), "only myeosgroupon is allowed to buy at this moment");

    require_auth(from);
    eosio_assert(eos.is_valid(), "Invalid token transfer");
    eosio_assert(eos.symbol == EOS_SYMBOL, "only system EOS token is allowed");
    eosio_assert(eos.amount > 0, "must buy a positive amount.");

    stringSplitter stream(memo);
    string operation;
    stream.get_string(&operation);

    if (operation == "buy") {        
        if (memo.size() > 7) {
            if (memo.substr(4, 3) == "for") {
                memo.erase(0, 8);
                name t( memo.c_str() ) ;
                eosio_assert( is_account(t), "Sponsor is not an existing account."); // sponsor 存在 check
            }
        }
        SEND_INLINE_ACTION( *this, buy, {_self ,"active"_n}, {from, eos} );
        // buy(from, eos);        
    } else {     
        /*   
        action(            
            permission_level{_self, N(active)},
            N(eosio.token), N(transfer),
                make_tuple(_self, N(minakokojima), eos, std::string("Unknown deposit."))
        ).send();    */    
    }
}

} /// namespace eosio

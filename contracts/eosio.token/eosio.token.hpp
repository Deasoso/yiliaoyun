/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class token : public contract {
      public:
      
         token( name receiver, name code, datastream<const char*> ds ) :
         contract( receiver, code, ds ){}

         ACTION init();
         
         ACTION create( name issuer,
                      asset        maximum_supply);

         ACTION issue( name to, asset quantity, string memo );

         ACTION transfer( name from,
                        name to,
                        asset        quantity,
                        string       memo );

         void onTransfer(name   from,
                    name   to,
                    asset          quantity,
                    string         memo);  

         ACTION buy(name account, asset supply) {    
            asset out;
            auto t = asset(supply.amount * 1000, S(4, MIC));

           // static char msg[20];
           // sprintf(msg, "delta: %llu", out.amount);
           // eosio_assert(false, msg);

            issue(account, out, "");
        }
      
         inline asset get_supply( symbol sym )const;
         
         inline asset get_balance( name owner, symbol sym )const;

      private:
         TABLE account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code.raw(); }
         };

         TABLE currency_stats {
            asset          supply;
            asset          max_supply;
            capi_name      issuer;

            uint64_t primary_key()const { return supply.symbol.code.raw(); }
         };

         typedef eosio::multi_index<"accounts"_n, account> accounts;
         typedef eosio::multi_index<"stat"_n, currency_stats> stats;

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value, name ram_payer );
        
      public:
         struct transfer_args {
            name  from;
            name  to;
            asset         quantity;
            string        memo;
         };
   };

   asset token::get_supply( symbol sym )const
   {
      stats statstable( get_self(), sym.code().raw() );
      const auto& st = statstable.get( sym.code().raw() );
      return st.supply;
   }

   asset token::get_balance( name owner, symbol sym )const
   {
      accounts accountstable( get_self(), owner.value );
      const auto& ac = accountstable.get( sym.code.raw() );
      return ac.balance;
   }

   void token::apply(uint64_t receiver, uint64_t code, uint64_t action) {
      auto &thiscontract = *this;
      if (action == ( "transfer"_n ).value && code == ( "eosio.token"_n ).value ) {
            auto transfer_data = unpack_action_data<st_transfer>();
            onTransfer(transfer_data.from, transfer_data.to, transfer_data.quantity, transfer_data.memo);
            return;
      }

      if (code != get_self().value) return;
      switch (action) {
            EOSIO_DISPATCH_HELPER(cryptojinian,
                  (create)(issue)(transfer)(init)
            )
      }
   }
}; /// namespace eosio

extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        eosio::token p( name(receiver), name(code), datastream<const char*>(nullptr, 0) );
        p.apply(receiver, code, action);
        eosio_exit(0);
    }
}
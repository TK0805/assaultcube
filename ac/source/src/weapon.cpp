// weapon.cpp: all shooting and effects code

#include "cube.h"

struct guninfo { short sound, reload, reloadtime, attackdelay, damage, projspeed, part, spread, recoil, magsize, mdl_kick_rot, mdl_kick_back; bool isauto; char *name; };

const int SGRAYS = 20;  //down from 20, must be 32 or less (default)
const float SGSPREAD = 2;
vec sg[SGRAYS];

//change the particales
guninfo guns[NUMGUNS] =
{    
    { S_KNIFE,    S_NULL,     0,      500,    50,     0,   0,  1,    1,   1,    0,  0,	false,	"knife"   },
    { S_PISTOL,   S_RPISTOL,  1400,   170,    20,     0,   0, 80,   10,   8,    6,  5,  false,	"pistol"  },  // *SGRAYS
    { S_SHOTGUN,  S_RSHOTGUN, 2400,   1000,   6,      0,   0,  1,   35,   7,    9,  9,  false,	"shotgun" },  //reload time is for 1 shell from 7 too powerful to 6
    { S_SUBGUN,   S_RSUBGUN,  1650,   80,     17,     0,   0, 70,   15,   30,   1,  2,  true,	"subgun"  },
    { S_SNIPER,   S_RSNIPER,  1950,   1500,   72,     0,   0, 60,   50,   5,    4,  4,  false,	"sniper"  },
    { S_ASSAULT,  S_RASSAULT,  2000,   130,    /*20*/25,     0,   0, 20,   40,   20,   0,  2,  true,	"assault"  },  //recoil was 44
    { S_NULL,	S_NULL,     1000,   1200,   150,    20,  6,  1,    1,   1,    3,  1,  "grenade" },
};

int nadetimer = 2000; // detonate after $ms
bool gun_changed = false;

void checkweaponswitch()
{
	if(!player1->weaponchanging) return;
    int timeprogress = lastmillis-player1->lastaction;
	if(timeprogress>WEAPONCHANGE_TIME) { player1->weaponchanging = false; player1->lastaction = 0; }
    else if(timeprogress>WEAPONCHANGE_TIME/2)
    {
        gun_changed = true;
        if((!player1->akimbo && player1->akimbomillis && player1->nextweapon==GUN_PISTOL) || 
            (player1->akimbo && !player1->akimbomillis && player1->gunselect==GUN_PISTOL)) 
                player1->akimbo = !player1->akimbo;
        player1->gunselect = player1->nextweapon;
    };
};

void weaponswitch(int gun)
{
    player1->weaponchanging = true;
    player1->lastaction = player1->akimbolastaction[0] = player1->akimbolastaction[1] = lastmillis;
    player1->nextweapon = gun;
	playsound(S_GUNCHANGE);
};

void weapon(int gun)
{
	if(m_nogun && gun!=G_MELEE && gun!=G_GRENADE) return;
	if(player1->weaponchanging) return;
    setscope(false);
	if(NADE_IN_HAND || player1->reloading) return;
    
    gun %= G_NUM;
    if(gun<0) gun = G_NUM+gun;
    switch(gun)
    {
        case G_PRIMARY: gun=player1->primary; break;
        case G_SECONDARY: gun=GUN_PISTOL;  break;
		case G_MELEE: gun=GUN_KNIFE; break;
		case G_GRENADE: if(player1->mag[GUN_GRENADE]) gun=GUN_GRENADE; else return;
    };
    
    if(gun!=player1->gunselect) weaponswitch(gun);
};

void shiftweapon(int i)
{
	if(player1->weaponchanging) return;
    int gun;
    switch(player1->gunselect)
    {
        case GUN_KNIFE: gun=G_MELEE; break;
        case GUN_PISTOL: gun=G_SECONDARY; break;
        case GUN_GRENADE: gun=G_GRENADE; break;
        default: gun=G_PRIMARY; break;
    };
	gun += i;
	weapon(gun);
	for(int j = 0; j <= G_NUM && !player1->weaponchanging; j++) { gun += i > 0 ? 1 : -1; weapon(gun); }
}

COMMAND(weapon, ARG_1INT);
COMMAND(shiftweapon, ARG_1INT);

VAR(scopefov, 5, 50, 50);
bool scoped = false;
int oldfov = 100;

void setscope(bool activate)
{
	if(activate == scoped) return;
	if(activate)
	{
		oldfov = getvar("fov");
		setvar("fov", scopefov);
	}
	else
	{
		setvar("fov", oldfov);
	}
	scoped = activate;
}

void altaction(int activate)
{
    //if(player1->gunselect == GUN_SNIPER && player1->state!=CS_DEAD) togglescope();
	if(player1->gunselect == GUN_SNIPER && player1->state!=CS_DEAD) setscope(activate!=0);
}

COMMAND(altaction, ARG_1INT);

void reload(playerent *d)
{
	if(!d || d->reloading || d->weaponchanging) return;   
	if(d == player1) setscope(false);
    bool akimbo = d->gunselect==GUN_PISTOL && d->akimbo!=0;
    
    if(d->gunselect==GUN_KNIFE || d->gunselect==GUN_GRENADE) return;
    if(akimbo && d->mag[d->gunselect]>=(guns[d->gunselect].magsize * 2)) return;
    else if(d->mag[d->gunselect]>=guns[d->gunselect].magsize && d->akimbo==0) return;
    if(d->ammo[d->gunselect]<=0) return;

    d->reloading = true;
    d->lastaction = lastmillis;
    d->akimbolastaction[0] = d->akimbolastaction[1] = lastmillis;

    d->gunwait = guns[d->gunselect].reloadtime;
    
    int a = guns[d->gunselect].magsize - d->mag[d->gunselect];
    if(d->gunselect==GUN_PISTOL && d->akimbo!=0)
	a = guns[d->gunselect].magsize * 2 - d->mag[d->gunselect];
    
    if (a >= d->ammo[d->gunselect])
    {
        d->mag[d->gunselect] += d->ammo[d->gunselect];
        d->ammo[d->gunselect] = 0;
    }
    else
    {
        d->mag[d->gunselect] += a;
        d->ammo[d->gunselect] -= a;
    }

    if(akimbo) playsoundc(S_RAKIMBO);
	else if(d->type==ENT_BOT) playsound(guns[d->gunselect].reload, &d->o);
	else playsoundc(guns[d->gunselect].reload);
};

void selfreload() { reload(player1); };
COMMANDN(reload, selfreload, ARG_NONE);

int reloadtime(int gun) { return guns[gun].reloadtime; };
int attackdelay(int gun) { return guns[gun].attackdelay; };
int magsize(int gun) { return guns[gun].magsize; };
int kick_rot(int gun) { return guns[gun].mdl_kick_rot; };
int kick_back(int gun) { return guns[gun].mdl_kick_back; };


void createrays(vec &from, vec &to)             // create random spread of rays for the shotgun
{
    float f = to.dist(from)*SGSPREAD/1000;
    loopi(SGRAYS)
    {
        #define RNDD (rnd(101)-50)*f
        vec r(RNDD, RNDD, RNDD);
        sg[i] = to;
        sg[i].add(r);
        #undef RNDD
    };
};

//bool intersect(dynent *d, vec &from, vec &to)   // if lineseg hits entity bounding box
bool intersect(dynent *d, vec &from, vec &to, vec *end)   // if lineseg hits entity bounding box
{
    vec v = to, w = d->o, *p; 
    v.sub(from);
    w.sub(from);
    float c1 = w.dot(v);

    if(c1<=0) p = &from;
    else
    {
        float c2 = v.squaredlen();
        if(c2<=c1) p = &to;
        else
        {
            float f = c1/c2;
            v.mul(f).add(from);
            p = &v;
        };
    };

    return p->x <= d->o.x+d->radius
        && p->x >= d->o.x-d->radius
        && p->y <= d->o.y+d->radius
        && p->y >= d->o.y-d->radius
        && p->z <= d->o.z+d->aboveeye
        && p->z >= d->o.z-d->eyeheight;
};

playerent *intersectclosest(vec &from, vec &to, int &n, playerent *at)
{
    playerent *best = NULL;
    float bestdist = 1e16f;
    if(at!=player1)
    {
        best = player1;
        bestdist = at->o.dist(player1->o);
    };
    loopv(players)
    {
        playerent *o = players[i];
        if(!o || o==at || o->state!=CS_ALIVE) continue;
        if(!intersect(o, from, to)) continue;
        float dist = at->o.dist(o->o);
        if(dist<bestdist)
        {
            best = o;
            bestdist = dist;
            n = i; 
        };
    };
    loopv(bots)
    {
        botent *o = bots[i];
        if(!o || o==at || o->state!=CS_ALIVE) continue;
        if(!intersect(o, from, to)) continue;
        float dist = at->o.dist(o->o);
        if(dist<bestdist)
        {
            best = o; 
            bestdist = dist;
            n = -i-1;
        };
    };
    return best;
};

playerent *playerincrosshair()
{
    if(demoplayback) return NULL;
    int n;
    return intersectclosest(player1->o, worldpos, n, player1);
};

const int MAXPROJ = 100;
struct projectile { vec o, to; float speed; playerent *owner; int gun; bool inuse, local; };
projectile projs[MAXPROJ];

void projreset() { loopi(MAXPROJ) projs[i].inuse = false; };

void newprojectile(vec &from, vec &to, float speed, bool local, playerent *owner, int gun)
{
    loopi(MAXPROJ)
    {
        projectile *p = &projs[i];
        if(p->inuse) continue;
        p->inuse = true;
        p->o = from;
        p->to = to;
        p->speed = speed;
        p->local = local;
        p->owner = owner;
        p->gun = gun;
        return;
    };
};

void hit(int target, int damage, playerent *d, playerent *at, bool gib=false)
{
    d->lastpain = lastmillis;
    if(d==player1) selfdamage(damage, at==player1 ? -1 : -2, at, gib);
    // Added by Rick: Let bots take damage
    else if(d->type==ENT_BOT)
    {
         if(((botent *)d)->pBot) ((botent *)d)->pBot->BotPain(damage, at, gib);
         else
         {
            int PlayerIndex = -1;
            if (at->type==ENT_BOT) PlayerIndex = BotManager.GetBotIndex((botent *)at);
            else
            {
                loopv(players)
                {
                    if (!players[i]) continue;

                    if (players[i] == at)
                    { 
                        PlayerIndex = i;
                        break;
                    }
                }
            }
         
            int msgtype = (at->type==ENT_BOT) ? SV_BOT2BOTDMG : SV_CLIENT2BOTDMG;
            addmsg(msgtype, "ri3", BotManager.GetBotIndex((botent *)d), damage, PlayerIndex);
            playsound(S_PAIN1+rnd(5), &d->o);
        };
    }
    // End add by Rick    
    //else { addmsg(SV_DAMAGE, "ri3", target, damage, d->lifesequence); playsound(S_PAIN1+rnd(5), &d->o); }; Modified by Rick: Added IsBot and PlayerIndex to message
    else
    {
         // Modified by Rick: Added IsBot and PlayerIndex to message
         int PlayerIndex = -1;
         if (at->type==ENT_BOT)
             PlayerIndex = BotManager.GetBotIndex((botent *)at);
         else
         {
             loopv(players)
             {
                 if (!players[i])
                     continue;

                  if (players[i] == at)
                  { 
                       PlayerIndex = i;
                       break;
                  }
             }
         }
         if(at->type==ENT_BOT) addmsg(SV_BOT2CLIENTDMG, "ri4", target, damage, d->lifesequence, PlayerIndex);
		 else addmsg(gib ? SV_GIBDAMAGE : SV_DAMAGE, "ri3", target, damage, d->lifesequence);
         playsound(S_PAIN1+rnd(5), &d->o);
    };
    
    particle_splash(3, damage, 1000, d->o);
    demodamage(damage, d->o);
};

const float RL_RADIUS = 7;
const float RL_DAMRAD = 10;   // hack

void radialeffect(playerent *o, vec &v, int cn, int qdam, playerent *at)
{
    if(o->state!=CS_ALIVE) return;
    vec temp;
    float dist = o->o.dist(v, temp);
    dist -= 2; // account for eye distance imprecision
    if(dist<RL_DAMRAD) 
    {
        if(dist<0) dist = 0;
        int damage = (int)(qdam*(1-(dist/RL_DAMRAD)));
        hit(cn, damage, o, at, true);
        o->vel.add(temp.mul((RL_DAMRAD-dist)*damage/800));
    };
};

// Modified by Rick: Added notthisbot
void splash(projectile *p, vec &v, vec &vold, int notthisplayer, int notthismonster, int notthisbot, int qdam)
{
    particle_splash(0, 50, 300, v);
    p->inuse = false;
    if(p->gun!=GUN_GRENADE)
    {
        playsound(S_FEXPLODE, &v);
        // no push?
    }
    else
    {
        //playsound(S_RLHIT, &v);
        newsphere(v, RL_RADIUS, 0);
        dodynlight(vold, v, 0, 0, p->owner);
        if(!p->local) return;
        radialeffect(player1, v, -1, qdam, p->owner);
        loopv(players)
        {
            if(i==notthisplayer) continue;
            playerent *o = players[i];
            if(!o) continue; 
            radialeffect(o, v, i, qdam, p->owner);
        };
        // Added by Rick
        loopv(bots)
        {
            if(i==notthisbot) continue;
            botent *o = bots[i];
            if(!o) continue; 
            radialeffect(o, v, i, qdam, p->owner);
        }        
        // End add by Rick                
        //dvector &mv = getmonsters();
        //loopv(mv) if(i!=notthismonster) radialeffect(mv[i], v, i, qdam, p->owner);
    };
};

// Modified by Rick
// inline void projdamage(dynent *o, projectile *p, vec &v, int i, int im, int qdam)
inline void projdamage(playerent *o, projectile *p, vec &v, int i, int im, int ib, int qdam)
{
    if(o->state!=CS_ALIVE) return;
    if(intersect(o, p->o, v))
    {
        // splash(p, v, p->o, i, im, qdam); Modified by Rick
        splash(p, v, p->o, i, im, ib, qdam);
        hit(i, qdam, o, p->owner, true); //fixme
    }; 
};

void moveprojectiles(float time)
{
    loopi(MAXPROJ)
    {
        projectile *p = &projs[i];
        if(!p->inuse) continue;
        //int qdam = guns[p->gun].damage*(p->owner->quadmillis ? 4 : 1);
        int qdam = guns[p->gun].damage;
        vec v;
        float dist = p->to.dist(p->o, v);
        float dtime = dist*1000/p->speed;
        if(time>dtime) dtime = time;
        v.mul(time/dtime).add(p->o);
        if(p->local)
        {
            loopv(players)
            {
                playerent *o = players[i];
                if(!o) continue; 
                // projdamage(o, p, v, i, -1, qdam); Modified by Rick
                projdamage(o, p, v, i, -1, -1, qdam);
            };
            // Added by Rick: Damage bots aswell
            loopv(bots)
            {
                botent *o = bots[i];
                if(!o || (o == p->owner)) continue;
                projdamage(o, p, v, -1, -1, i, qdam);
            };
            // End add by Rick            
            // if(p->owner!=player1) projdamage(player1, p, v, -1, -1, qdam); Modified by Rick
            if(p->owner!=player1) projdamage(player1, p, v, -1, -1, -1, qdam);
            //dvector &mv = getmonsters();
            //loopv(mv) if(!vreject(mv[i]->o, v, 10.0f) && mv[i]!=p->owner) projdamage(mv[i], p, v, -1, i, qdam); Modified by Rick
            //loopv(mv) if(!vreject(mv[i]->o, v, 10.0f) && mv[i]!=p->owner) projdamage(mv[i], p, v, -1, i, -1, qdam);            
        };
        if(p->inuse)
        {
            // if(time==dtime) splash(p, v, p->o, -1, -1, qdam); Modified by Rick
            if(time==dtime) splash(p, v, p->o, -1, -1, -1, qdam);
            else
            {
                if(p->gun==GUN_GRENADE) { dodynlight(p->o, v, 0, 255, p->owner); particle_splash(5, 2, 200, v); }
                else { particle_splash(1, 1, 200, v); particle_splash(guns[p->gun].part, 1, 1, v); };
                // Added by Rick
                traceresult_s tr;
                TraceLine(p->o, v, p->owner, true, &tr);
                if (tr.collided) splash(p, v, p->o, -1, -1, -1, qdam);
                // End add                
            };       
        };
        p->o = v;
    };
};

void throw_nade(playerent *d, vec &to, bounceent *p)
{
    if(!p || !d) return;
    playsound(S_GRENADETHROW);

    p->bouncestate = NADE_THROWED;
    
    p->o = d->o;
    p->vel.z = sinf(RAD*d->pitch);
    float speed = cosf(RAD*d->pitch);
    p->vel.x = sinf(RAD*d->yaw)*speed;
    p->vel.y = -cosf(RAD*d->yaw)*speed;
    
    p->vel.mul(1.7f);

    vec throwdir = p->vel;
    throwdir.mul(2*d->radius);
    p->o.add(throwdir);

    d->thrownademillis = lastmillis;
    d->inhandnade = NULL;
    
    if(d==player1)
    {
        vec &from = d->o;
		player1->lastaction = lastmillis;
        addmsg(SV_SHOT, "ri8", d->gunselect, (int)(from.x*DMF), (int)(from.y*DMF), (int)(from.z*DMF), (int)(to.x*DMF), (int)(to.y*DMF), (int)(to.z*DMF), lastmillis-p->millis);
    };
};

bounceent *new_nade(playerent *d, int millis = 0)
{
    bounceent *p = newbounceent();
    p->owner = d;
    p->millis = lastmillis;
    p->timetolife = 2000-millis;
    p->bouncestate = NADE_ACTIVATED;
    
    d->inhandnade = p;
    d->thrownademillis = 0;  
	playsound(S_GRENADEPULL, (d==player1 ? NULL : &d->o));
    return p;
};

void explode_nade(bounceent *i)
{ 
    if(!i) return;
    
    if(i->bouncestate != NADE_THROWED)
    {   
        vec o = i->owner->o;
        o.add(0.1f);
        throw_nade(i->owner, o, i);
    };
    playsound(S_FEXPLODE, &i->o);
    newprojectile(i->o, i->o, 1, i->owner==player1, i->owner, GUN_GRENADE);
};

void shootv(int gun, vec &from, vec &to, playerent *d, bool local, int nademillis)     // create visual effect from a shot
{
    playsound(guns[gun].sound, d==player1 ? NULL : &d->o);
    switch(gun)
    {
        case GUN_KNIFE:
            break;

        case GUN_SHOTGUN:
        {
            loopi(SGRAYS) particle_splash(0, 5, 200, sg[i]);
            break;
        };

        case GUN_PISTOL:
        case GUN_SUBGUN:
        case GUN_ASSAULT:
            addshotline(d, from, to);
            particle_splash(0, 5, 250, to);
            //particle_trail(1, 10, from, to);
            break;

        case GUN_GRENADE:
		{
			if(d!=player1)
			{
				/*int nademillis = nadetimer/100*nadepercent;
				if(nademillis<0 || nademillis>100) nademillis = 100;*/
				bounceent *p = new_nade(d, nademillis);
				throw_nade(d, to, p);
			}
		};
            break;

        case GUN_SNIPER: 
            addshotline(d, from, to);
            particle_splash(0, 50, 200, to);
            particle_trail(1, 500, from, to);
            break;
    };
};

void hitpush(int target, int damage, playerent *d, playerent *at, vec &from, vec &to)
{
	hit(target, damage, d, at, at->gunselect==GUN_KNIFE ? true : false);
    vec v;
    float dist = to.dist(from, v);
    v.mul(damage/dist/50);
    d->vel.add(v);
};

void shorten(vec &from, vec &to, vec &target)
{
    target.sub(from).normalize().mul(from.dist(to)).add(from);
};

void raydamage(vec &from, vec &to, playerent *d)
{
    int i = -1, gdam = guns[d->gunselect].damage;;
    playerent *o = NULL;
    if(d->gunselect==GUN_SHOTGUN)
    {
        uint done = 0;
        playerent *cl = NULL;
        int n = -1;
        for(;;)
        {
            bool raysleft = false;
            int damage = 0;
            o = NULL;
            loop(r, SGRAYS) if((done&(1<<r))==0 && (cl = intersectclosest(from, sg[r], n, d)))
            {
                if(!o || o==cl)
                {
                    damage += gdam;
                    o = cl;
                    done |= 1<<r;
                    i = n;
                    shorten(from, o->o, sg[r]);
                }
                else raysleft = true;
            };
            if(damage) hitpush(i, damage, o, d, from, to);
            if(!raysleft) break;
        };
    }
    else if((o = intersectclosest(from, to, i, d)))
    {
        hitpush(i, gdam, o, d, from, to);
        shorten(from, o->o, to);
    };
}; 

void spreadandrecoil(vec &from, vec &to, playerent *d)
{
    //nothing special for a knife
    if (d->gunselect==GUN_KNIFE || d->gunselect==GUN_GRENADE) return;

    //spread
    vec unitv;
    float dist = to.dist(from, unitv);
    float f = dist/1000;
    int spd = guns[d->gunselect].spread;

    //recoil
    float rcl = guns[d->gunselect].recoil*-0.01f;

    if(d->gunselect==GUN_ASSAULT)
    {
        if(d->shots > 3)
            spd = 70;
        rcl += (rnd(8)*-0.01f);
    };

    if((d->gunselect==GUN_SNIPER) && (d->vel.x<.25f && d->vel.y<.25f) && scoped)
    {
        spd = 1;
        rcl = rcl / 3;
    };

    if(d->gunselect!=GUN_SHOTGUN)  //no spread on shotgun
    {   
        #define RNDD (rnd(spd)-spd/2)*f
        vec r(RNDD, RNDD, RNDD);
        to.add(r);
        #undef RNDD
    };

   //increase pitch for recoil
    d->vel.add(vec(unitv).mul(rcl/dist));

    if(d->pitch<80.0f) d->pitch += guns[d->gunselect].recoil*0.05f;
};

bool hasammo(playerent *d) 	// bot mod
{
	if(!d->mag[d->gunselect])
	{
		if (d->type==ENT_BOT) botplaysound(S_NOAMMO, (botent *)d);
		else playsoundc(S_NOAMMO);
		d->gunwait = 250;
		d->lastattackgun = -1;
		return false;
	} else return true;
}

const int grenadepulltime = 650;
bool akimboside = false;

void shoot(playerent *d, vec &targ)
{
	int attacktime = lastmillis-d->lastaction;

	vec from = d->o;
	vec to = targ;

	if(d->gunselect==GUN_GRENADE)
	{
		d->shots = 0;

		if(d->thrownademillis && attacktime >= 13*(1000/25))
		{
			d->weaponchanging = true;
			d->nextweapon = d->mag[GUN_GRENADE] ? GUN_GRENADE : d->primary;
			d->thrownademillis = 0;
			d->lastaction = lastmillis-1-WEAPONCHANGE_TIME/2;
		};

		if(d->weaponchanging || attacktime<d->gunwait) return;
		d->gunwait = 0;
		d->reloading = false;

		if(d->attacking && !d->inhandnade) // activate
		{
			if(!hasammo(d)) return;
			d->mag[d->gunselect]--;
			new_nade(d);
			d->gunwait = grenadepulltime;
			d->lastaction = lastmillis;
			d->lastattackgun = d->gunselect;
		}
		else if(!d->attacking && d->inhandnade && attacktime>grenadepulltime) // throw
		{
			throw_nade(d, targ, d->inhandnade);
		}
		return;
	}
	else
	{
		if(d->weaponchanging || attacktime<d->gunwait) return;
		d->gunwait = 0;
		d->reloading = false;

		if(!d->attacking) { d->shots = 0; return; };
		d->lastaction = lastmillis;
		d->lastattackgun = d->gunselect;

		if(!hasammo(d)) { d->shots = 0; return; };
			
		if(guns[d->gunselect].isauto) d->shots++;
		else d->attacking = false;

		if(d->gunselect==GUN_PISTOL && d->akimbo) 
		{
			d->attacking = true;  //make akimbo auto
			d->akimbolastaction[akimboside?1:0] = lastmillis;
			akimboside = !akimboside;
		}
	    
		if(d->gunselect) d->mag[d->gunselect]--;
		from.z -= 0.2f;    // below eye
	};
	
	spreadandrecoil(from,to,d);
    vec unitv;
    float dist = to.dist(from, unitv);
    unitv.div(dist);

	if(d->gunselect==GUN_KNIFE) 
	{
        unitv.mul(3); // punch range
		to = from;
        to.add(unitv);
	};   
	if(d->gunselect==GUN_SHOTGUN) createrays(from, to);
	
	if(has_akimbo(d)) d->gunwait = guns[d->gunselect].attackdelay / 2;  //make akimbo pistols shoot twice as fast as normal pistol
	else d->gunwait = guns[d->gunselect].attackdelay;
	
	shootv(d->gunselect, from, to, d, 0);
	addmsg(SV_SHOT, "ri8", d->gunselect, (int)(from.x*DMF), (int)(from.y*DMF), (int)(from.z*DMF), (int)(to.x*DMF), (int)(to.y*DMF), (int)(to.z*DMF), 0 );
	if(guns[d->gunselect].projspeed || d->gunselect==GUN_GRENADE) return;

    raydamage(from, to, d);	
};

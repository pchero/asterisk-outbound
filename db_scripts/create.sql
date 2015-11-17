-- create.sql
-- Created on: Nov 10, 2015
--     Author: pchero

drop table if exists plan;
create table plan(

    -- identity
    uuid        varchar(255) unique not null, 
    name        varchar(255),           -- plan name
    detail      varchar(1023),          -- description
    
    -- resource
--    trunk_group     varchar(255),       -- trunk_group_ma uuid.
    uui_field       varchar(255),       -- x-header name for UUI
    
    -- strategy
    dial_mode       varchar(255),       -- dial mode(desktop, power, predictive, email, fax, sms)
    dial_timeout    int default 30000,  -- no answer hangup timeout(30000 ms = 30 second)
    caller_id       varchar(255),       -- caller name(from)
    answer_handle   varchar(255),       -- answer handling.(all, human_only, human_possible)
    dl_end_handle   varchar(255),       -- stratery when it running out dial list(keep_running, stop)
    retry_delay     varchar(255),       -- retry delaytime(ms)
    queue_name      varchar(255),       -- queue name
    trunk_name      varchar(255),       -- trunk name
    
    -- retry number
    max_retry_cnt_1     int default 5,  -- max retry count for dial number 1
    max_retry_cnt_2     int default 5,  -- max retry count for dial number 2
    max_retry_cnt_3     int default 5,  -- max retry count for dial number 3
    max_retry_cnt_4     int default 5,  -- max retry count for dial number 4
    max_retry_cnt_5     int default 5,  -- max retry count for dial number 5
    max_retry_cnt_6     int default 5,  -- max retry count for dial number 6
    max_retry_cnt_7     int default 5,  -- max retry count for dial number 7
    max_retry_cnt_8     int default 5,  -- max retry count for dial number 8

    -- ownership
    create_agent_uuid           varchar(255),       -- create agent uuid
    delete_agent_uuid           varchar(255),       -- delete agent uuid
    update_property_agent_uuid  varchar(255),       -- last propery update agent uuid
    
    -- timestamp. UTC.
    tm_create           datetime(6),    -- create time.
    tm_delete           datetime(6),    -- delete time.
    tm_update_property  datetime(6),    -- last property update time.(Except status)
    tm_update_status    datetime(6),    -- last status updated time.
    
    primary key(uuid)
);

-- dial list original.
drop table if exists dl_org;
create table dl_org(
-- original dial list info table
-- all of other dial lists are copy of this table.

    -- identity
    uuid        varchar(255)    unique,     -- dl uuid
    
    -- information
    name            varchar(255),                   -- Can be null
    detail          varchar(255),
    uui             text,                           -- user-user information
    status          varchar(255) default "idle",    -- dial list status. ("idle", "dialing", ...)
    
    -- current dialing
    dialing_camp_uuid       varchar(255),       -- dialing campaign_uuid
    dialing_chan_unique_id  varchar(255),       -- dialing channel unique id.
    
    -- numbers.
    number_1    varchar(255),       -- tel number 1
    number_2    varchar(255),       -- tel number 2
    number_3    varchar(255),       -- tel number 3
    number_4    varchar(255),       -- tel number 4
    number_5    varchar(255),       -- tel number 5
    number_6    varchar(255),       -- tel number 6
    number_7    varchar(255),       -- tel number 7
    number_8    varchar(255),       -- tel number 8
    
    -- other address.
    email       text,

    -- try counts.
    trycnt_1    int default 0,      -- try count for tel number 1
    trycnt_2    int default 0,      -- try count for tel number 2
    trycnt_3    int default 0,      -- try count for tel number 3
    trycnt_4    int default 0,      -- try count for tel number 4
    trycnt_5    int default 0,      -- try count for tel number 5
    trycnt_6    int default 0,      -- try count for tel number 6
    trycnt_7    int default 0,      -- try count for tel number 7
    trycnt_8    int default 0,      -- try count for tel number 8

    -- last dial result
    res_dial        varchar(255),   -- last dial result.(no answer, answer, busy, ...)
    res_hangup      varchar(255),   -- last route result after answer.(routed, agent busy, no route place, ...)
--    call_detail text,               -- more detail info about call result

    -- timestamp. UTC.
    tm_create       datetime(6),   -- create time
    tm_delete       datetime(6),   -- delete time
    tm_update       datetime(6),   -- last update time
    tm_last_dial    datetime(6),   -- last tried dial time
    
    -- ownership
    create_agent_uuid           varchar(255),       -- create agent uuid
    delete_agent_uuid           varchar(255),       -- delete agent uuid
    update_property_agent_uuid  varchar(255),       -- last propery update agent uuid

    primary key(uuid)
);

CREATE TRIGGER init_uuid BEFORE INSERT ON dl_org
  FOR EACH ROW SET NEW.uuid = UUID();

drop table if exists dial_list_ma;
create table dial_list_ma(
-- dial list
-- manage all of dial list tables

    -- row identity
    seq         int(10)         unsigned auto_increment,    -- sequence
    uuid        varchar(255)    unique,                     -- dial_list_#### reference uuid.
    
    -- information
    name        varchar(255),                               -- dial list name
    dl_table    varchar(255),                               -- dial list table name.(dl_e276d8be)
    detail      text,                                       -- description of dialist
    
    -- timestamp. UTC.
    tm_create           datetime(6),    -- create time.
    tm_delete           datetime(6),    -- delete time.
    tm_update_property  datetime(6),    -- last property update time.(Except status)
    
    -- ownership
    create_agent_uuid           varchar(255),       -- create agent uuid
    delete_agent_uuid           varchar(255),       -- delete agent uuid
    update_property_agent_uuid  varchar(255),       -- last propery update agent uuid
    
    primary key(seq, uuid)
);

drop table if exists campaign;
create table campaign(
-- campaign.
    
    -- identity
    uuid        varchar(255)    unique,
    
    -- information
    detail                      varchar(1023),      -- description
    name                        varchar(255),       -- campaign name
    status      varchar(10)     default "stop",     -- status(stop/start/starting/stopping/force_stopping)
    status_code int             default 0,          -- status code(stop(0), start(1), pause(2), stopping(10), starting(11), pausing(12)
    
    next_campaign               varchar(255),       -- next campaign uuid
    
    -- ownership
    create_agent_uuid             varchar(255),       -- create agent uuid
    delete_agent_uuid             varchar(255),       -- delete agent uuid
    update_property_agent_uuid    varchar(255),       -- last propery update agent uuid
    update_status_agent_uuid      varchar(255),       -- last status update agent uuid
    
    -- resources
--    agent_group varchar(255),                       -- agent group uuid
    plan    varchar(255),                       -- plan uuid
    dlma    varchar(255),                       -- dial_list_ma uuid
--    trunk_group varchar(255),                       -- trunk group uuid -- will be removed.

    -- timestamp. UTC.
    tm_create           datetime(6),   -- create time.
    tm_delete           datetime(6),   -- delete time.
    tm_update_property  datetime(6),   -- last property update time.(Except status)
    tm_update_status    datetime(6),   -- last status updated time.
        
    foreign key(plan)   references plan(uuid)           on delete set null on update cascade,
    foreign key(dlma)   references dial_list_ma(uuid)   on delete set null on update cascade,
--    foreign key(trunk_group)    references trunk_group_ma(uuid) on delete set null on update cascade,
    
    primary key(uuid)
);

drop table if exists campaign_result;
create table campaign_result(
-- campaign dial result table.
    -- identity
    seq                 int(10)         unsigned auto_increment,
    chan_unique_id      varchar(255)    not null,   -- channel unique id.
    dl_uuid             varchar(255)    not null,   -- dl uuid
    dlma_uuid           varchar(255)    not null,   -- dial_list_ma uuid.
    camp_uuid           varchar(255)    not null,   -- campaign uuid.

    -- dial_info
    info_camp   text    not null,   -- campaign info. json format.
    info_plan   text    not null,   -- plan info. json format.
    info_dl     text    not null,   -- dl info. json format.    
    
    -- timestamp(UTC)
    tm_dial             datetime(6),   -- timestamp for dialing requested.
    tm_dial_end         datetime(6),   -- timestamp for dialing ended.
    
    tm_redirect         datetime(6),   -- timestamp for call redirected.
    tm_bridge           datetime(6),   -- timestamp for call bridged.
    tm_hangup           datetime(6),   -- timestamp for call hanguped.
    
    tm_tr_dial          datetime(6),   -- timestamp for dialing to agent.
    tm_tr_dial_end      datetime(6),   -- timestamp for transfer to agent.
    tm_tr_hangup        datetime(6),   -- timestamp for agent hangup.

    -- dial info
    dial_index          int,            -- dialing number index.
    dial_addr           varchar(255),   -- dialing address.
    dial_trycnt         int,            -- dialing try count.
    dial_timeout        int,            -- dialing timeout.
    
    -- transfer info
    tr_trycnt          int,             -- transfer try count.
    tr_agent_uuid        varchar(255),    -- transfered agent.
    tr_chan_unique_id  varchar(255),    -- trying transfer chan unique id.
    
    -- dial result
    res_dial                varchar(255),   -- dial result(answer, no_answer, ...)
    res_answer              varchar(255),   -- AMD result.(AMDSTATUS)
    res_answer_detail       varchar(255),   -- AMD result detail.(AMDCAUSE)
    res_hangup              varchar(255),   -- hangup code.
    res_hangup_detail       varchar(255),   -- hangup detail.
    res_tr_dial             varchar(255),   -- transferred dial result(answer, no_answer, ...)
    res_tr_hangup           varchar(255),   -- hangup code.
    res_tr_hangup_detail    varchar(255),   -- hangup detail.
        
    primary key(seq, chan_unique_id)
    
);

-- insert plan
insert into plan(uuid, name, dial_mode, answer_handle, queue_name) values ("5ad6c7d8-535c-4cd3-b3e5-83ab420dcb56", "sample_plan", "predictive", "all", "TestQueue");

-- create dial list
drop table if exists dl_e276d8be;
create table dl_e276d8be like dl_org;
CREATE TRIGGER init_uuid BEFORE INSERT ON dl_e276d8be FOR EACH ROW SET NEW.uuid = UUID();
insert into dial_list_ma(uuid, name, dl_table) values ("e276d8be-a558-4546-948a-f99913a7fea2", "sample_dial_list", "dl_e276d8be");

-- insert dial list
insert into dl_e276d8be(name, number_1) values ("test1", "111-111-0001");
insert into dl_e276d8be(name, number_1) values ("test2", "111-111-0002");
insert into dl_e276d8be(name, number_1) values ("test3", "111-111-0003");
insert into dl_e276d8be(name, number_1) values ("test4", "111-111-0004");


-- insert campaign
insert into campaign(uuid, name, status, plan, dlma) 
values (
"8cd1d05b-ad45-434f-9fde-4de801dee1c7", "sample_campaign", "start", "5ad6c7d8-535c-4cd3-b3e5-83ab420dcb56", "e276d8be-a558-4546-948a-f99913a7fea2"
);

commit;
-- SET @s := CONCAT('SELECT * FROM ', (SELECT  `dl_list` FROM `dial_list_ma` WHERE `uuid` = 'dl-e276d8be-a558-4546-948a-f99913a7fea2'));  PREPARE stmt FROM @s;  EXECUTE stmt;  DEALLOCATE PREPARE stmt; //;


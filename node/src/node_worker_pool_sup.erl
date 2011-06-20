%% @author Ben Nortier <bjnortier@shapesmith.net>
%% @copyright 2011 ben Nortier.

%% @doc Supervisor for the node application.

-module(node_worker_pool_sup).
-author('Ben Nortier <bjnortier@shapesmith.net>').

-behaviour(supervisor).

%% External exports
-export([start_link/0, upgrade/0]).

%% supervisor callbacks
-export([init/1]).

%% @spec start_link() -> ServerRet
%% @doc API for starting the supervisor.
start_link() ->
    supervisor:start_link({local, ?MODULE}, ?MODULE, []).

%% @spec upgrade() -> ok
%% @doc Add processes if necessary.
upgrade() ->
    {ok, {_, Specs}} = init([]),

    Old = sets:from_list(
            [Name || {Name, _, _, _} <- supervisor:which_children(?MODULE)]),
    New = sets:from_list([Name || {Name, _, _, _, _, _} <- Specs]),
    Kill = sets:subtract(Old, New),

    sets:fold(fun (Id, ok) ->
                      supervisor:terminate_child(?MODULE, Id),
                      supervisor:delete_child(?MODULE, Id),
                      ok
              end, ok, Kill),

    [supervisor:start_child(?MODULE, Spec) || Spec <- Specs],
    ok.

%% @spec init([]) -> SupervisorTree
%% @doc supervisor callback.
init([]) ->

    {ok, WorkerPoolSize} = application:get_env(node, worker_pool_size),
    {ok, WorkerMaxTime} = application:get_env(node, worker_max_time),
    
    WorkerSup = {node_worker_sup,
		  {node_worker_sup, start_link, []},
		  permanent, 5000, worker, [node_worker_sup]},

    WorkerPool = {node_worker_pool,
		  {node_worker_pool, start_link, [WorkerPoolSize, WorkerMaxTime]},
		  permanent, 5000, worker, [node_worker_pool]},

    {ok,{{one_for_all,10,10}, [WorkerSup, WorkerPool]}}. 


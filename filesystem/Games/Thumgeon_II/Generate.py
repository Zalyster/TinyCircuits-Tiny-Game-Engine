from engine_resources import NoiseResource
import urandom
import utime

import Tiles
import Player

adjacency_likelihoods = {
    0: (0.75, 0.125, 7*0.125/8, 0.125/8), # Grass0
    1: (0.125, 0.75, 7*0.125/8, 0.125/8), # Grass1
    2: (0.125, 0.125, 0.5, 0.25),		  # Stone
    3: (0.125, 0.125, 0.5, 0.25),		  # Cracked stone
}

def shuffle(arr):
    for i in range(len(arr)-1, 0 -1):
        j = urandom.randrange(i+1)
        arr[i], arr[j] = arr[j], arr[i]

@micropython.native
def generate_left(tilemap, x, y):
    global adjacency_likelihoods
    id = tilemap.get_tile_id(x, y)
    if((x > 0) and tilemap.get_tile_id(x-1, y) == 255): # Generate left tile
        sum = adjacency_likelihoods[id][0]
        p = urandom.random()
        adj = 0
        while(sum < p and adj < len(adjacency_likelihoods[id])-1):
            adj += 1
            sum += adjacency_likelihoods[id][adj]
        tilemap.tiles[(y*tilemap.WIDTH+x-1)*3] = adj
        
@micropython.native
def generate_right(tilemap, x, y):
    global adjacency_likelihoods
    id = tilemap.get_tile_id(x, y)
    if((x < tilemap.WIDTH) and tilemap.get_tile_id(x+1, y) == 255): # Generate right tile
        sum = adjacency_likelihoods[id][0]
        p = urandom.random()
        adj = 0
        while(sum < p and adj < len(adjacency_likelihoods[id])-1):
            adj += 1
            sum += adjacency_likelihoods[id][adj]
        tilemap.tiles[(y*tilemap.WIDTH+x+1)*3] = adj

@micropython.native
def generate_top(tilemap, x, y):
    global adjacency_likelihoods
    id = tilemap. get_tile_id(x, y)
    if((y > 0) and tilemap.get_tile_id(x, y-1) == 255): # Generate top tile
        sum = adjacency_likelihoods[id][0]
        p = urandom.random()
        adj = 0
        while(sum < p and adj < len(adjacency_likelihoods[id])-1):
            adj += 1
            sum += adjacency_likelihoods[id][adj]
        tilemap.tiles[((y-1)*tilemap.WIDTH+x)*3] = adj

@micropython.native
def generate_bottom(tilemap, x, y):
    global adjacency_likelihoods
    id = tilemap.get_tile_id(x, y)
    if((y < tilemap.HEIGHT) and tilemap.get_tile_id(x, y+1) == 255): # Generate bottom tile
        sum = adjacency_likelihoods[id][0]
        p = urandom.random()
        adj = 0
        while(sum < p and adj < len(adjacency_likelihoods[id])-1):
            adj += 1
            sum += adjacency_likelihoods[id][adj]
        tilemap.tiles[((y+1)*tilemap.WIDTH+x)*3] = adj


@micropython.native
def generate_tiles(tilemap):
    fs = [generate_left, generate_right, generate_top, generate_bottom]
    for y in range(0, tilemap.HEIGHT):
        for x in range(0, tilemap.WIDTH):
            if(int(tilemap.get_tile_id(x, y)) == 255):
                # Generate random tile
                tilemap.tiles[(y*tilemap.WIDTH+x)*3] = urandom.randrange(4)
            shuffle(fs)
            # Each tile only generates two neighbors
            for i in range(3):
                fs[i](tilemap, x, y)


perlin = NoiseResource()
perlin.seed = utime.ticks_us()

@micropython.native
def generate_water(tilemap):
    for y in range(0, tilemap.HEIGHT):
        for x in range(0, tilemap.WIDTH):
            if(perlin.noise_2d(x*10, y*10) < 0.001):
                tilemap.set_tile_id(x, y, Tiles.tile_ids["water1"])
                #tilemap.set_tile_data1(x, y, 255)
                tilemap.set_tile_solid(x, y, True)

@micropython.native
def generate_deco(tilemap):
    for y in range(0, tilemap.HEIGHT):
        for x in range(0, tilemap.WIDTH):
            tile_id = tilemap.get_tile_id(x, y)
            if(tile_id == Tiles.tile_ids["grass1"] or tile_id == Tiles.tile_ids["grass2"]):
                # 10% chance of having grass decoration
                if(urandom.random() < 0.3):
                    tilemap.set_tile_data0(x, y, Tiles.deco_ids["grass_patch"])
                    tilemap.set_deco_under(x, y, False)
            elif(tile_id == Tiles.tile_ids["stone1"]):
                # 10% chance of having grass decoration
                if(urandom.random() < 0.1):
                    tilemap.set_tile_data0(x, y, Tiles.deco_ids["door_sheet"])
                    tilemap.set_deco_under(x, y, True)

@micropython.native
def generate_empty_dungeon(tilemap):
    for y in range(1, tilemap.HEIGHT-1):
        for x in range(1, tilemap.WIDTH-1):
            tilemap.set_tile_id(x, y, Tiles.tile_ids["stonefloor1"])
    for y in range(0, tilemap.HEIGHT):
        tilemap.set_tile_id(0, y, Tiles.tile_ids["stone1"])
        tilemap.set_tile_data1(0, y, 1)
        tilemap.set_tile_id(tilemap.WIDTH-1, y, Tiles.tile_ids["stone1"])
        tilemap.set_tile_data1(tilemap.WIDTH-1, y, 1)
    for x in range(1, tilemap.WIDTH-1):
        tilemap.set_tile_id(x, 0, Tiles.tile_ids["stone1"])
        tilemap.set_tile_data1(x, 0, 1)
        tilemap.set_tile_id(x, tilemap.HEIGHT-1, Tiles.tile_ids["stone1"])
        tilemap.set_tile_data1(x, tilemap.HEIGHT-1, 1)
    
    exit_side = urandom.randrange(4)
    if exit_side == 0:
        door_offset = urandom.randrange(1, tilemap.WIDTH-1)
        tilemap.set_tile_data0(door_offset, 0, Tiles.deco_ids["door_sheet"])
        tilemap.set_deco_under(door_offset, 0, True)
    elif exit_side == 1:
        door_offset = urandom.randrange(1, tilemap.WIDTH-1)
        tilemap.set_tile_data0(door_offset, tilemap.HEIGHT-1, Tiles.deco_ids["door_sheet"])
        tilemap.set_deco_under(door_offset, tilemap.HEIGHT-1, True)
    elif exit_side == 2:
        door_offset = urandom.randrange(1, tilemap.HEIGHT-1)
        tilemap.set_tile_data0(0, door_offset, Tiles.deco_ids["door_sheet"])
        tilemap.set_deco_under(0, door_offset, True)
    elif exit_side == 3:
        door_offset = urandom.randrange(1, tilemap.HEIGHT-1)
        tilemap.set_tile_data0(0, door_offset, Tiles.deco_ids["door_sheet"])
        tilemap.set_deco_under(0, door_offset, True)
        
def generate_dungeon_level(tilemap):
    exit_x = urandom.randrange(1, tilemap.WIDTH-1)
    exit_y = urandom.randrange(1, tilemap.HEIGHT-1)
    tilemap.set_tile_data0(exit_x, exit_y, Tiles.deco_ids["trapdoor_sheet"])
    tilemap.set_deco_under(exit_x, exit_y, True)
    for i in range(3):
        item_x = urandom.randrange(1, tilemap.WIDTH-1)
        item_y = urandom.randrange(1, tilemap.HEIGHT-1)
        if(tilemap.get_tile_data0(item_x, item_y) == 0):
            item = urandom.randrange(len(Player.item_ids))
            tilemap.set_tile_data0(item_x, item_y, item)
            tilemap.set_tile_data1(item_x, item_y, 4)
        #tilemap.set_deco_under(exit_x, exit_y, True)
    
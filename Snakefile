configfile:
    "config.yml"


rule all:
    input:
        "results/animations/test.gif"


rule generate_timeline:
    output:
        "intermediate/{name}.newick"
    params:
        end_time = config["end_time"],
        birth_rate = config["birth_rate"],
        death_rate = config["death_rate"],
        birth_interaction = config["birth_interaction"],
        death_interaction = config["death_interaction"],
        mutation_rate = config["mutation_rate"],
        starting_population = config["starting_population"]
    shell:
        """
        code/bin/dmut -u {params.mutation_rate} \
                      -t {params.end_time} \
                      -b '{params.birth_rate}' \
                      -d '{params.death_rate}' \
                      -p '{params.birth_interaction}' \
                      -n '{params.starting_population}' \
                      -q '{params.death_interaction}' > {output}
        """


rule generate_particles:
    input:
        "intermediate/{name}.newick"
    output:
        "intermediate/{name}.particles"
    params:
        max_velocity = config["max_velocity"],
        max_acceleration = config["max_acceleration"],
        friction = config["friction"],
        timestep = config["timestep"],
        end_time = config["end_time"]
    shell:
        """
        code/bin/particles -i {input} \
                           -a {params.max_acceleration} \
                           -r {params.friction} \
                           -d {params.timestep} \
                           -t {params.end_time} \
                           -v {params.max_velocity} > {output}
        """


rule generate_animation:
    input:
        "intermediate/{name}.particles"
    output:
        "results/animations/{name}.gif"
    params:
        frame_skip = config["frame_skip"],
        frame_delay = config["frame_delay"],
        width = config["width"],
        height = config["height"]
    shell:
        """
        code/bin/metaballs -i {input} \
                           -o {output} \
                           -f {params.frame_skip} \
                           -d {params.frame_delay} \
                           -x {params.width} \
                           -y {params.height}
        """
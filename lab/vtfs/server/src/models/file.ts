import { Repository } from "./repository";

export class File extends Repository {
    static tableName = "files";
    static columns = [
        { name: "ino", primary: true },
        { name: "parent_ino" },
        { name: "name" },
        { name: "is_dir" },
        { name: "data" },
    ];

    ino!: number;
    parent_ino!: number;
    name!: string;
    is_dir!: boolean;
    data!: Buffer | null;
}

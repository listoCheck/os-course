import {Context, Next} from "koa";

export async function create(ctx: Context, next: Next) {

    await next();
}